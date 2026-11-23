#include "state/client_receive.h"
#include "state/client_validation.h"
#include "state/client_plan.h"
#include "state/mod_codec.h"
#include "state_host_fields.inc"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <string>

namespace cs::state {
namespace {
[[noreturn]] void Fail(StateReadiness r,const char *why){throw ClientShadowError(r,why);}
bool SameTicket(StateApplyTicket a,StateApplyTicket b){return a.serial==b.serial&&a.route==b.route;}
unsigned Slot(const StateTarget &t){return t.count==1&&t.sections[0].stream==BodyStream::World&&t.sections[0].base_version?0:1;}
auto BodyPeriod(){return std::chrono::microseconds((1000000+StateBudgets().body_hz-1)/StateBudgets().body_hz);}
// Body 等待位期限：两个 Body 周期，覆盖错过一次 Manager 钩子（增量应用设计 10.4.1）。
auto BodyWaitLimit(){return 2*BodyPeriod();}
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
  ModSchemaRef mod_schema;
  ModShadowRef mod_shadow;
  unsigned mod_ordinal=kModPublic;
  std::uint64_t mod_rejected=0;
  bool active=false,recovery_requested=false;
  bool uncertain_event=false;
  bool slice_pending=false;
  StatePendingCandidates pending;
  // 计划的引用和游标固定在原等待槽，Body 可在同一 worker 上即时使用小工作区。
  struct PrepareWork {
    ClientStatePreparedTarget target;
    std::shared_ptr<ClientTargetCheckWork> checking;
    std::shared_ptr<ClientInstallPlanWork> planning;
    std::shared_ptr<ClientStateNativePlanWork> describing;
    bool filtered_resources=false;
    explicit PrepareWork(ClientStatePreparedTarget value):target(std::move(value)) {}
  };
  // 普通 World 与联合各有一个候选槽。数字头只用于定界，不授予 Ready。
  struct Waiting {
    ClientStateVector<std::byte> bytes;
    StateLane lane=StateLane::World;
    ClientStateCandidateRef candidate;
    ModShadowRef mods;
    std::shared_ptr<ClientShadowBuildWork> decoding;
    StateTarget target;
    std::optional<PrepareWork> preparing;
    std::chrono::steady_clock::time_point deadline;
    std::uint64_t received_us=0;
#ifdef ISAACCS_E2E
    // 只在耗尽后台工作片时起表，正常等待资源不计入片间空隙。
    std::uint64_t slice_end_us=0;
#endif
    explicit Waiting(const std::shared_ptr<ClientStateMemory> &m):bytes(ClientStateAllocator<std::byte>(m,memory::Pool::Wire)) {}
  };
  std::array<std::optional<Waiting>,2> waiting;
  // 一个较新的 World 可在已有解码片结束后接管，避免连续片间收包丢掉最后状态。
  std::optional<Waiting> next_world;
  // 新增普通Body原始包位；不持有候选／表版本，也不占第二个应用票据。
  std::optional<Waiting> body_waiting;
  // 回执和物理借用与票据同寿命。World／Body各一槽，联合与恢复独占。
  struct Work {
    std::shared_ptr<ClientStateApplyBatch> batch;
    bool delivered=false,awaiting_drain=false;
    std::uint16_t receipt_count=0;
    std::uint64_t receipt_uncertain=0;
    bool receipt_applied=false;
  };
  std::array<Work,2> work;
  // 只认真正接受的本流计划。弱借用不让旧对侧目录因模板复用继续占用快照。
  std::array<std::weak_ptr<const ClientStateInstallPlan>,2> accepted_plans;
  // 成功反馈不代表主线程和Render已经放下计划；最终析构只在后台发生。
  ClientStateVector<ClientStateApplyBatchRef> retired;
  struct ResourceWork {
    ClientStateResourceJob job;
    bool delivered=false;
  };
  std::array<std::optional<ResourceWork>,kClientResourceJobsInFlight> resources;
  // 可靠目录一到就排预载，候选帧只借用同一缓存的结果。
  ClientStateVector<ClientStateResourceNeed> preload;
  std::size_t preload_cursor=0;
  std::uint64_t last_table_sequence=0,ack_due=0,resync_due=0;
  struct BadFrame {std::string reason;unsigned count=0;};
  std::array<BadFrame,2> bad_frames;
  std::uint8_t flat_recovery=0;
  Impl(std::shared_ptr<ClientStateMemory> m,std::span<const ClientShadowLayoutBinding> registry,
       const ClientStatePreparation &p,ClientStateReceiveDelivery &d)
    :memory(std::move(m)),bindings(ClientStateAllocator<ClientMemberLayoutBinding>(memory)),
     tables(memory,thread,TableBindings(registry)),shadow(memory,thread,registry),events(memory),preparation(p),delivery(d),retired(ClientStateAllocator<ClientStateApplyBatchRef>(memory)),
     preload(ClientStateAllocator<ClientStateResourceNeed>(memory)) {}
  std::span<const ClientMemberLayoutBinding> TableBindings(std::span<const ClientShadowLayoutBinding> input) {
    bindings.reserve(input.size());for(const auto &v:input)bindings.push_back({v.space,v.layout});return bindings;
  }
  void Thread() const {if(thread!=std::this_thread::get_id())throw ClientTableError(ClientTableFailure::WrongThread,"client_receive_wrong_thread");}
  bool Occupied(std::uint8_t mask) const {
    return std::any_of(work.begin(),work.end(),[&](const auto &slot){
      return slot.batch&&(StateApplyOccupancy(slot.batch->target.candidate->Target())&mask);
    });
  }
  bool JointWaiting() const {return waiting[1]&&StateApplyOccupancy(waiting[1]->target)==3;}
  bool Blocked(const StateTarget &target) const {
    const auto mask=StateApplyOccupancy(target);
    return Occupied(mask)||(mask!=3&&JointWaiting());
  }
  // 先让已进入原生写段的批次正常反馈。仅原子撤回仍在准备中的票据。
  void WithdrawForJoint() {
    for(auto &slot:work)if(slot.batch&&StateApplyOccupancy(slot.batch->target.candidate->Target())!=3){
      auto expected=ClientStateWriteState::Preparing;
      if(slot.batch->write_state.compare_exchange_strong(expected,ClientStateWriteState::Withdrawn,
          std::memory_order_acq_rel,std::memory_order_acquire))
        slot.batch->cancelled.store(true,std::memory_order_release);
    }
  }
  void Collect() {
    for(auto i=retired.begin();i!=retired.end();) {
      // native与target.described是批次内的两份引用；派生owner还须检查所有子描述。
      if(i->use_count()==1&&(*i)->native.use_count()<=2&&(*i)->native->Releasable())i=retired.erase(i);else ++i;
    }
    shadow.Collect();tables.Collect();
    memory->ApplyMetrics().BackgroundHolds(unsigned(bool(waiting[0]))+unsigned(bool(waiting[1]))+
      unsigned(bool(body_waiting))+unsigned(bool(next_world)),
      unsigned(bool(work[0].batch))+unsigned(bool(work[1].batch)),retired.size());
    if(memory->ApplyMetrics().TakeBlockSampleRequest()){
      std::array<ClientStateCandidateRef,2> candidates;
      std::array<std::shared_ptr<ClientShadowBuildWork>,2> decoding;
      for(unsigned i=0;i<2;++i)if(waiting[i]){candidates[i]=waiting[i]->candidate;decoding[i]=waiting[i]->decoding;}
      shadow.SampleBlockMemory(candidates,decoding);
    }
  }
  void Recover(std::uint8_t streams,StateResyncReason reason,bool structural=false) {
    if(reason==StateResyncReason::Budget)memory->ApplyMetrics().Wait(streams,ClientApplyWait::Budget);
    structural|=streams==3;
    for(unsigned i=0;i<2;++i)if((structural?3:streams)&(1u<<i))accepted_plans[i].reset();
    if(structural||(streams&2))body_waiting.reset();
    if(structural||(streams&1))next_world.reset();
    shadow.RequireRecovery(streams,reason,structural);
    for(const auto &slot:work)if(slot.batch&&(StateTargetStreams(slot.batch->target.candidate->Target())&(structural?3:streams)))
      slot.batch->cancelled.store(true,std::memory_order_release);
    for(unsigned i=0;i<2;++i)if(waiting[i]&&(StateTargetStreams(waiting[i]->target)&(structural?3:streams))){pending.Remove(i);waiting[i].reset();}
    resync_due=0;
    recovery_requested=true;
  }
  struct Validation final:ClientShadowValidation {
    const Impl &owner;ClientStatePreparedTarget &target;
    Validation(const Impl &o,ClientStatePreparedTarget &t):owner(o),target(t) {}
    StateReadiness Inspect(const ClientStateCandidate &c) const override {
      if(target.candidate.get()!=&c)return StateReadiness::InvalidEncoding;
      if(!target.install||!target.described)return StateReadiness::SourcePending;
      return owner.preparation.Inspect(target,owner.tables);
    }
  };
  ClientStatePreparedTarget Target(const ClientStateCandidateRef &candidate, ModShadowRef mods={}) {
    ClientStatePreparedTarget out(memory);out.candidate=candidate;out.mods=std::move(mods);
    out.mod_lifecycle=mod_schema&&mod_schema->declared;
    for(unsigned i=0;i<2;++i){out.previous[i]=shadow.Installed(BodyStream(i));out.effective[i]=candidate->State(BodyStream(i))?candidate->State(BodyStream(i)):out.previous[i];}
    const auto world=candidate->State(BodyStream::World);
    if(world)out.events=events.Prepare(*candidate,world->Context().game_frame);
    return out;
  }
  void Reack(const StateTarget *rejected=nullptr) {
    // 只计候选拒绝；周期重发 ACK 不计丢帧。旧基线与已接受目标的重传分开。
    if(rejected){
      const auto versions=shadow.ReceivedVersions();bool newer=false;
      for(unsigned i=0;i<rejected->count;++i){const auto &s=rejected->sections[i];newer|=s.target_version>versions[unsigned(s.stream)];}
      memory->ApplyMetrics().Wait(StateTargetStreams(*rejected),newer?ClientApplyWait::OldBase:ClientApplyWait::Duplicate);
    }
    std::array<std::byte,256> bytes{};
    for(const auto &ack:shadow.Reacknowledgements())if(ack)
      if(!delivery.Control(StateMessageKind::AppliedAck,EncodeStateAppliedAckInto(*ack,bytes)))break;
  }
  void ReceiveCandidate(const ClientStateCandidateRef &candidate,std::uint64_t received_us) {
    // 事件先接管进本地有界队列，才能随接收快照一起推进水位。
    events.Receive(candidate);
    if(!candidate->Frame().mod_results.empty()){
      net::ModIntentResults results;
      if(ReadModResults(candidate->Frame().mod_results,results)){
        for(unsigned i=0;i<results.count;++i){
          auto &result=results.values[i];
          result.connection=route.connection;result.run=route.scope.run;result.floor=route.scope.floor;
          delivery.ModResult(result);
        }
      }else memory->ApplyMetrics().FirstIssue(1,{},"mod_decode","mod_result_rejected");
    }
    const auto ack=shadow.RetainReceived(candidate);
    // 字段验证及影子分配失败只影响 Mod。ACK 和理由仍归原生接收管线。
    if(const auto world=candidate->State(BodyStream::World);world&&mod_schema){
      const auto rejected=mod_rejected;
      try {
        unsigned players=0;
        if(!candidate->Frame().mod_values.empty()||mod_shadow)
          for(std::size_t i=0;i<world->RecordCount();++i){
            const auto record=world->Record(i);
            if(record.Identity().layout!=host_field::PlayerControlGroup::kLayout)continue;
            const auto *ordinal=record.Field(host_field::PlayerControlGroup::ordinal);
            if(ordinal&&ordinal->present&&ordinal->bytes.size()==1){
              const auto slot=std::to_integer<unsigned>(ordinal->bytes.front());
              if(slot<4)players|=1u<<slot;
            }
          }
        mod_shadow=MergeModShadow(mod_shadow,*mod_schema,candidate->Frame().mod_values,mod_ordinal,players,world->Members()->Directory().entries,
          !world->Context().base_version,mod_rejected);
      }catch(...){++mod_rejected;}
      if(rejected!=mod_rejected){
        memory->ApplyMetrics().Volume(1,ClientApplyVolume::ModRecordsRejected,mod_rejected-rejected);
        memory->ApplyMetrics().FirstIssue(1,{},"mod_decode","mod_record_rejected");
      }
    }
    const auto streams=StateTargetStreams(candidate->Target());
    for(unsigned i=0;i<2;++i)if(streams&(1u<<i))bad_frames[i]={};
    for(unsigned i=0;i<2;++i)if(const auto state=candidate->State(BodyStream(i))){
      flat_recovery&=std::uint8_t(~(1u<<i));
      for(const auto &fault:state->FlatFaults()){
        ClientApplyMetrics::Issue issue;
        issue.route=candidate->Target().route.route;issue.member=fault.identity.id;
        issue.layout=fault.layout;issue.field=fault.field;
        memory->ApplyMetrics().FirstIssue(std::uint8_t(1u<<i),issue,"flat_decode",fault.reason.data());
        if(fault.count>=StateBudgets().decode_failure_limit)flat_recovery|=std::uint8_t(1u<<i);
      }
    }
    std::array<std::byte,256> bytes{};
    if(delivery.Control(StateMessageKind::AppliedAck,EncodeStateAppliedAckInto(ack,bytes)))
      // 与接收至原生发布共用收包时刻，分别统计两个终点。
      memory->ApplyMetrics().Mark(streams,ClientApplyStage::Ack,ClientApplyMetrics::Now()-received_us);
  }
  void InvalidFrame(std::uint8_t streams,const char *reason) {
    std::uint8_t recover=0;
    for(unsigned i=0;i<2;++i)if(streams&(1u<<i)){
      auto &failure=bad_frames[i];
      if(failure.reason!=reason){failure.reason=reason;failure.count=0;}
      const auto limit=StateBudgets().decode_failure_limit;
      if(failure.count<limit&&++failure.count==limit)recover|=std::uint8_t(1u<<i);
    }
    memory->ApplyMetrics().FirstIssue(streams,{},"decode",reason);
    if(recover)Recover(recover,StateResyncReason::Dependency);
  }
  bool Dispatch(ClientStatePreparedTarget &target,std::chrono::steady_clock::time_point deadline,std::uint64_t received) {
    const auto candidate=target.candidate;const auto &descriptor=candidate->Target();const auto occupancy=StateApplyOccupancy(descriptor);
    if(Blocked(descriptor))return false;
    // 计划相对实际current生成；不能在Begin时用更新版本替换它比较过的基线。
    for(unsigned i=0;i<2;++i)if((StateTargetStreams(descriptor)&(1u<<i))&&
        target.previous[i]!=shadow.Installed(BodyStream(i))){
      memory->ApplyMetrics().Wait(StateTargetStreams(descriptor),ClientApplyWait::Stale);return true;
    }
    Validation validation(*this,target);
    const auto decision=shadow.Assess(candidate,validation);
    if(decision.action==StateAcceptAction::Wait||decision.action==StateAcceptAction::Decode)return false;
    if(decision.action==StateAcceptAction::Terminal)Fail(StateReadiness::InvalidEncoding,"client_receive_terminal_candidate");
    if(decision.action==StateAcceptAction::Resync){Recover(decision.streams,decision.reason);return false;}
    if(decision.action==StateAcceptAction::Apply) {
      // 所有分配／描述生成在Begin之前完成；主线程看不到尚未封存的计划。
      auto batch=std::allocate_shared<ClientStateApplyBatch>(ClientStateAllocator<ClientStateApplyBatch>(memory),memory);
      batch->target=std::move(target);batch->native=batch->target.described;
      batch->deadline=deadline;batch->received_us=received;
      if(!batch->native)Fail(StateReadiness::SourcePending,"client_receive_missing_native_plan");
      // 退休索引也在Begin前预留，完成反馈阶段不分配。
      retired.push_back(batch);Validation final_validation(*this,batch->target);
      try{batch->ticket=shadow.Begin(candidate,final_validation);}catch(...){retired.pop_back();throw;}
      batch->application_bases=shadow.HistoricalVersions();
      if(occupancy==2&&descriptor.sections[0].base_version!=batch->application_bases[1])
        memory->ApplyMetrics().Wait(2,ClientApplyWait::BodyRebased);
      // 基线仅在后台确定变化与结构时使用。主线程持有完整新引用表，
      // 不能让已安装写入器多固定一代只用于比较的表与块。
      batch->target.previous={};
      auto &slot=work[StateApplyChannel(descriptor)];slot.batch=batch;
      batch->admitted_us=ClientApplyMetrics::Now();
      memory->ApplyMetrics().Mark(StateTargetStreams(descriptor),ClientApplyStage::Admit);

      slot.delivered=delivery.Apply(batch); // 邮箱忙保留原票据，Pump重试。
      if(!slot.delivered)memory->ApplyMetrics().Wait(StateTargetStreams(descriptor),ClientApplyWait::Mailbox);
    }else if(decision.action==StateAcceptAction::Reack)Reack(&descriptor);
    return true;
  }
  void Try(unsigned slot,std::chrono::steady_clock::time_point stop) {
    if(!waiting[slot]||!waiting[slot]->candidate||Blocked(waiting[slot]->target))return;
    auto &queued=*waiting[slot];
#ifdef ISAACCS_E2E
    ResumeSlice(queued);
#endif
    if(!queued.preparing)try{queued.preparing.emplace(Target(shadow.ForWorldApplication(queued.candidate),queued.mods));}
      catch(const ClientShadowError &e){PreparationFailed(StateTargetStreams(queued.target),e.Readiness());return;}
    if(!AdvancePreparation(*queued.preparing,stop))return;
    if(!Dispatch(queued.preparing->target,queued.deadline,queued.received_us))return;
    pending.Remove(slot);waiting[slot].reset();
  }
#ifdef ISAACCS_E2E
  void ResumeSlice(Waiting &slot) noexcept {
    if(!slot.slice_end_us)return;
    memory->ApplyMetrics().Mark(StateTargetStreams(slot.target),ClientApplyStage::BackgroundGap,ClientApplyMetrics::Now()-slot.slice_end_us);
    slot.slice_end_us=0;
  }
#endif
  bool ResourcesPending(std::span<const ClientStateResourceNeed> needed) const {
    for(const auto &need:needed) {
      const auto state=tables.ResourceState(need.resources->Directory().epoch,need.key);
      if(state==ClientResourceState::Unprepared||state==ClientResourceState::Pending)return true;
      if(state==ClientResourceState::Failed||state==ClientResourceState::MissingLocalResource)return false;
    }
    return false;
  }
  bool ResourcesInFlight() const noexcept {
    return std::any_of(resources.begin(),resources.end(),[](const auto &work){return work.has_value();});
  }
  bool DeliverResources() {
    for(auto &work:resources)if(work&&!work->delivered){
      if(!delivery.Resource(work->job))return false;
      work->delivered=true;
    }
    return true;
  }
  void RequestResources(std::span<const ClientStateResourceNeed> needed) {
    if(!DeliverResources())return;
    for(const auto &need:needed)if(tables.ResourceState(need.resources->Directory().epoch,need.key)==ClientResourceState::Unprepared) {
      const auto slot=std::find_if(resources.begin(),resources.end(),[](const auto &work){return !work;});
      if(slot==resources.end())return;
      // 全部解码／分配先于票据；异常不留下没有主线程消费者的 Pending 借用。
      const auto *entry=FindResource(need.resources->Directory(),need.key);
      if(!entry)Fail(StateReadiness::MissingClosure,"client_receive_resource_description_missing");
      std::array<ProjectionFact,64> facts{};ResourceCodecScratch scratch{{facts}};
      auto description=std::allocate_shared<ResourceLocalDescription>(ClientStateAllocator<ResourceLocalDescription>(memory),DescribeResourceLocal(*entry,scratch));
      ClientStateResourceJob job{tables.BeginResourcePreparation(need.resources->Directory().epoch,need.key),need.resources,std::move(description)};
      slot->emplace(ResourceWork{std::move(job)});
      if(!DeliverResources())return;
    }
  }
  void QueueResourcePreload() {
    const auto snapshot=tables.LatestResources();
    if(!snapshot)return;
    const auto directory=snapshot->Directory();
    const auto count=directory.entries.size();
    ClientStateVector<ResourceKey> roots{ClientStateAllocator<ResourceKey>(memory,memory::Pool::Workspace)};
    ClientStateVector<std::uint32_t> order{ClientStateAllocator<std::uint32_t>(memory,memory::Pool::Workspace)};
    ClientStateVector<std::uint8_t> state{ClientStateAllocator<std::uint8_t>(memory,memory::Pool::Workspace)},heights(state.get_allocator());
    roots.reserve(count);order.resize(count);state.resize(count);heights.resize(count);
    for(const auto &entry:directory.entries)roots.push_back(entry.key);
    ResourceGraphScratch scratch{{},{},order,state,heights};
    const auto graph=tables.Prepare(snapshot,roots,scratch);
    preload.clear();preload_cursor=0;
    preload.reserve(graph.materialization_order.size());
    for(const auto index:graph.materialization_order)preload.push_back({snapshot,directory.entries[index].key});
  }
  void PumpResourcePreload() {
    while(preload_cursor<preload.size()){
      const auto &need=preload[preload_cursor];
      RequestResources(std::span(&need,1));
      if(tables.ResourceState(need.resources->Directory().epoch,need.key)==ClientResourceState::Unprepared)return;
      ++preload_cursor;
    }
    preload.clear();preload_cursor=0;
  }
  bool PreparationFailed(std::uint8_t streams,StateReadiness state) {
    switch(state){
      case StateReadiness::ResourcePending:
        memory->ApplyMetrics().Wait(streams,ClientApplyWait::Resources);return false;
      case StateReadiness::SourcePending:case StateReadiness::MissingTable:
        memory->ApplyMetrics().Wait(streams,ClientApplyWait::Dependency);return false;
      case StateReadiness::MissingClosure:Recover(streams,StateResyncReason::Table);return false;
      case StateReadiness::EventGap:Recover(streams,StateResyncReason::Event);return false;
      case StateReadiness::BaseLost:Recover(streams,StateResyncReason::Base);return false;
      case StateReadiness::Budget:Recover(streams,StateResyncReason::Budget);return false;
      case StateReadiness::Lifecycle:Recover(streams,StateResyncReason::Lifecycle);return false;
      default:Fail(state,"client_receive_terminal_candidate");
    }
  }
  bool AdvancePreparation(PrepareWork &work,std::chrono::steady_clock::time_point stop) {
    auto &target=work.target;const auto streams=StateTargetStreams(target.candidate->Target());
    auto &metrics=memory->ApplyMetrics();
    try{
      if(!target.facts){
        ClientApplyTimer time(metrics,streams,ClientApplyStage::Inspect);
        target.facts=InspectClientStateTargetStep(target,tables,work.checking,stop);
        if(!target.facts){slice_pending|=bool(work.checking);return false;}
      }
      RequestResources(target.facts->resources);
      if(ResourcesPending(target.facts->resources)){
        if(!work.filtered_resources){
          work.filtered_resources=true;
          if(auto applied=shadow.DeferUnreadyObjects(target.candidate,tables)){
            target=Target(applied,target.mods);slice_pending=true;return false;
          }
        }
        metrics.LastWait(streams,{},"resources","resource_materialization_pending");
        metrics.Wait(streams,ClientApplyWait::Resources);return false;
      }
      if(std::chrono::steady_clock::now()>=stop){slice_pending=true;return false;}
      if(!target.install){
        ClientApplyTimer time(metrics,streams,ClientApplyStage::Plan);
        target.install=BuildClientStateInstallPlanStep(target,tables,work.planning,stop,accepted_plans);
        if(!target.install||std::chrono::steady_clock::now()>=stop){slice_pending=true;return false;}
      }
      if(!target.described){
        ClientApplyTimer time(metrics,streams,ClientApplyStage::Describe);
        target.described=preparation.PrepareStep(target,work.describing,stop);
        slice_pending|=bool(work.describing);
      }
      return bool(target.described);
    }catch(const ClientShadowError &e){
      ClientApplyMetrics::Issue waiting;waiting.route=route.route;waiting.layout=e.LayoutId();
      waiting.field=e.FieldId();waiting.member=e.Member();
      metrics.LastWait(streams,waiting,target.install?"describe":target.facts?"plan":"validate",e.what());
      if(e.Readiness()!=StateReadiness::SourcePending&&e.Readiness()!=StateReadiness::ResourcePending&&e.Readiness()!=StateReadiness::MissingTable){
        ClientApplyMetrics::Issue detail;detail.route=route.route;detail.layout=e.LayoutId();
        detail.field=e.FieldId();detail.member=e.Member();
        metrics.FirstIssue(streams,detail,target.install?"describe":target.facts?"plan":"validate",e.what());
      }
      return PreparationFailed(streams,e.Readiness());
    }
     catch(const ClientTableError &e){
      if(e.Failure()==ClientTableFailure::WorkspaceBusy)return false;
      if(e.Failure()!=ClientTableFailure::Budget&&e.Failure()!=ClientTableFailure::SnapshotLimit)throw;
      return PreparationFailed(streams,StateReadiness::Budget);
    }
  }
  std::optional<ClientShadowBuildResult> Decode(StateLane lane,std::span<const std::byte> bytes,std::uint8_t streams,
      std::shared_ptr<ClientShadowBuildWork> &work,std::chrono::steady_clock::time_point stop) {
    ClientApplyTimer time(memory->ApplyMetrics(),streams,ClientApplyStage::Decode);
    try{return shadow.BuildStep(route,lane,bytes,tables,work,stop);}
    catch(const ClientShadowError &e) {
      StateResyncReason reason=StateResyncReason::Dependency;
      switch(e.Readiness()) {
        case StateReadiness::MissingTable:case StateReadiness::ResourcePending:case StateReadiness::SourcePending:
          return ClientShadowBuildResult{{StateAcceptAction::Wait,streams,reason},{}};
        case StateReadiness::InvalidEncoding:case StateReadiness::InvalidReference:throw;
        case StateReadiness::MissingClosure:Fail(StateReadiness::InvalidReference,e.what());
        case StateReadiness::EventGap:reason=StateResyncReason::Event;break;
        case StateReadiness::BaseLost:reason=StateResyncReason::Base;break;
        case StateReadiness::Budget:reason=StateResyncReason::Budget;break;
        case StateReadiness::Lifecycle:reason=StateResyncReason::Lifecycle;break;
        default:throw;
      }
      return ClientShadowBuildResult{{StateAcceptAction::Resync,streams,reason},{}};
    }catch(const ClientTableError &e) {
      if(e.Failure()==ClientTableFailure::InvalidEncoding||e.Failure()==ClientTableFailure::EpochChain||
          e.Failure()==ClientTableFailure::InvalidReference)Fail(StateReadiness::InvalidEncoding,e.what());
      if(e.Failure()==ClientTableFailure::WorkspaceBusy)return {};
      if(e.Failure()!=ClientTableFailure::Budget&&e.Failure()!=ClientTableFailure::SnapshotLimit)throw;
      return ClientShadowBuildResult{{StateAcceptAction::Resync,streams,StateResyncReason::Budget},{}};
    }
  }
  // 等待期限属于槽首次到达；重复和更新替换均不能让旧工作无限续命。
  // 定时检查只在一次派发尝试之后进行（见 Pump），不抢在可派发的最新 Body 之前丢弃它。
  void ExpireBody() {
    if(body_waiting&&std::chrono::steady_clock::now()>=body_waiting->deadline){
      memory->ApplyMetrics().Wait(2,ClientApplyWait::BodyQueueExpired);body_waiting.reset();
    }
  }
  void QueueBody(const StateTarget &descriptor,std::span<const std::byte> bytes,std::uint64_t received) {
    auto &metrics=memory->ApplyMetrics();ExpireBody();
    const auto decision=shadow.AssessEnvelope(descriptor,tables);
    if(decision.action==StateAcceptAction::Terminal)Fail(StateReadiness::InvalidEncoding,"client_receive_conflicting_target");
    if(decision.action==StateAcceptAction::Reack){Reack(&descriptor);return;}
    if(decision.action==StateAcceptAction::Resync){Recover(decision.streams,decision.reason);return;}
    const auto version=descriptor.sections[0].target_version;
    // 正在提交的相同／更新目标不再排一次；冲突已由接纳上下文检查。
    if(body_waiting){
      if(StateTargetsConflict(body_waiting->target,descriptor))Fail(StateReadiness::InvalidEncoding,"client_receive_conflicting_body_waiting");
      if(version<=body_waiting->target.sections[0].target_version)return;
      body_waiting->decoding.reset();body_waiting->candidate.reset();body_waiting->preparing.reset();
      metrics.Wait(2,ClientApplyWait::BodyReplaced);
    }else{
      body_waiting.emplace(memory);body_waiting->lane=StateLane::Body;
      body_waiting->deadline=std::chrono::steady_clock::now()+BodyWaitLimit();
    }
    auto &queued=*body_waiting;queued.bytes.assign(bytes.begin(),bytes.end());
    queued.target=descriptor;queued.target.encoded=queued.bytes;queued.received_us=received;
    metrics.Wait(2,ClientApplyWait::BodyQueued);
  }
  // 即使 Manager 正忙也接收并确认 Body。只有将纯值交给原生的阶段占用票据。
  // 过期检查不在这里做：先尝试派发，Pump 随后才判过期。
  void TryBody() {
    if(!body_waiting)return;
    auto &queued=*body_waiting;
    if(!queued.candidate){
      const auto decoded=Decode(StateLane::Body,queued.bytes,2,queued.decoding,std::chrono::steady_clock::time_point::max());
      if(!decoded)return;
      const auto &built=*decoded;
      if(!built.candidate){
        if(built.decision.action==StateAcceptAction::Wait)return;
        const auto descriptor=queued.target;
        if(built.decision.action==StateAcceptAction::Reack)Reack(&descriptor);
        body_waiting.reset();
        if(built.decision.action==StateAcceptAction::Terminal)InvalidFrame(2,"client_receive_conflicting_body");
        if(built.decision.action==StateAcceptAction::Resync)Recover(built.decision.streams,built.decision.reason);
        return;
      }
      ReceiveCandidate(built.candidate,queued.received_us);queued.candidate=built.candidate;
      queued.target=built.candidate->Target();queued.bytes.clear();
    }
    // 被本流在途票据挡住时排队等它完成，不顶替已交出的票据（10.4.1 已撤销该条）。
    if(Blocked(queued.target))return;
    if(!queued.preparing)queued.preparing.emplace(Target(shadow.SelectKnownBodyObjects(queued.candidate)));
    if(AdvancePreparation(*queued.preparing,std::chrono::steady_clock::time_point::max())&&
        Dispatch(queued.preparing->target,std::chrono::steady_clock::now()+BodyPeriod(),queued.received_us))
      body_waiting.reset();
  }
  void Store(StateLane lane,std::span<const std::byte> bytes,std::uint64_t now) {
    const auto received=ClientApplyMetrics::Now();
    const auto world_deadline=std::chrono::steady_clock::now()+std::chrono::milliseconds(StateBudgets().world_wait_ms);
    // 小型定界scratch足以处理正式最大成员数；typed正文仍由shadow独立有界解码。
    ClientStateVector<StateRecordPayload> records{ClientStateAllocator<StateRecordPayload>(memory,memory::Pool::Workspace)};
    ClientStateVector<StateEventView> ev{ClientStateAllocator<StateEventView>(memory,memory::Pool::Workspace)};
    ClientStateVector<std::uint16_t> order{ClientStateAllocator<std::uint16_t>(memory,memory::Pool::Workspace)};
    const auto header=UsesFlatRecordContract(LayoutVersion())?kFlatRecordHeaderBytes:12;
    records.resize(std::min<std::size_t>(65535,bytes.size()/header));ev.resize(512);order.resize(records.size());
    StateFrameScratch scratch{records,ev,order,{}};
    const auto frame=InspectStateFrame(route.scope,bytes,shadow.ReceivedEvents(),scratch,{std::uint32_t(StateMessageBodyLimit(lane)),262144,32,lane==StateLane::Baseline});
    if(StateFrameLane(frame)!=lane)Fail(StateReadiness::InvalidEncoding,"client_receive_frame_lane");
    const auto descriptor=DescribeStateTarget(route,frame);
    const auto streams=StateTargetStreams(descriptor);
    auto &metrics=memory->ApplyMetrics();metrics.Mark(streams,ClientApplyStage::Receive,ClientApplyMetrics::Now()-received);
    // 联合帧只在 World 页记一次，普通帧在实际流的页中计数。
    if(descriptor.count==2)metrics.Volume(1,ClientApplyVolume::JointFrames);
    else metrics.Volume(streams,ClientApplyVolume::SingleFrames);
    const auto occupancy=StateApplyOccupancy(descriptor);
    const bool fast=occupancy==2;
    const bool blocked=Blocked(descriptor);
    if(blocked)metrics.Wait(streams,JointWaiting()||occupancy==3?
      ClientApplyWait::Joint:ClientApplyWait::OwnStream);
    const auto slot=Slot(descriptor);
    if(fast) {
      QueueBody(descriptor,bytes,received);return;
    }
    const auto decision=shadow.AssessEnvelope(descriptor,tables);
    if(decision.action==StateAcceptAction::Terminal)Fail(StateReadiness::InvalidEncoding,"client_receive_conflicting_target");
    if(decision.action==StateAcceptAction::Reack){Reack(&descriptor);return;}
    if(decision.action==StateAcceptAction::Resync){Recover(streams,decision.reason);return;}
    if(waiting[slot]&&StateTargetsConflict(waiting[slot]->target,descriptor))
      Fail(StateReadiness::InvalidEncoding,"client_receive_conflicting_target");
    const bool decoding=waiting[slot]&&waiting[slot]->decoding;
    if(decoding&&slot!=0)return;
    Waiting value(memory);value.lane=lane;value.received_us=received;
    value.bytes.assign(bytes.begin(),bytes.end());value.target=descriptor;value.target.encoded=value.bytes;
    const auto versions=shadow.HistoricalVersions();
    if(blocked&&occupancy==3)for(unsigned i=0;i<descriptor.count;++i)
      if(descriptor.sections[i].target_version<=versions[unsigned(descriptor.sections[i].stream)])return;
    // 普通 World 的期限包含先前处理耗时，可靠基线从进入等待槽计时。
    value.deadline=slot==0?world_deadline:
      std::chrono::steady_clock::now()+std::chrono::milliseconds(StateBudgets().baseline_wait_ms);
    if(decoding){
      const auto version=value.target.sections[0].target_version;
      if(version<=waiting[slot]->target.sections[0].target_version)return;
      if(next_world){
        if(StateTargetsConflict(next_world->target,value.target))Fail(StateReadiness::InvalidEncoding,"client_receive_conflicting_next_world");
        if(version<=next_world->target.sections[0].target_version)return;
      }
      next_world.emplace(std::move(value));return;
    }
    if(!pending.Offer(value.target,now))return;
    waiting[slot].emplace(std::move(value));
    if(occupancy==3){
      body_waiting.reset();next_world.reset();
      pending.Remove(0);waiting[0].reset();
      WithdrawForJoint();
    }
  }
};
ClientStateReceiver::ClientStateReceiver(std::shared_ptr<ClientStateMemory> memory,std::span<const ClientShadowLayoutBinding> registry,
    const ClientStatePreparation &p,ClientStateReceiveDelivery &d):impl_(std::allocate_shared<Impl>(ClientStateAllocator<Impl>(memory),memory,registry,p,d)) {}
ClientStateReceiver::~ClientStateReceiver()=default;
void ClientStateReceiver::Open(const StateRoute &route,StateAgreement agreement,std::uint64_t generation,std::uint64_t,
    ModSchemaRef mods,unsigned ordinal) {
  auto &s=*impl_;s.Thread();s.Collect();if(s.active||s.Occupied(3)||s.ResourcesInFlight()||!s.retired.empty()||!s.preparation.Available()||!s.shadow.Drained()||!s.tables.Drained()||!CompatibleStateAgreement(agreement))Fail(StateReadiness::Lifecycle,"client_receive_open_busy_or_native_unavailable");
  s.tables.Open(route,agreement,generation);s.shadow.Open(route,agreement);s.events.Open(route);s.route=route;s.active=true;
  s.mod_schema=std::move(mods);s.mod_ordinal=ordinal;s.mod_shadow.reset();s.mod_rejected=0;
  s.last_table_sequence=0;
  s.bad_frames={};s.flat_recovery=0;s.ack_due=s.resync_due=0;
  s.recovery_requested=s.uncertain_event=s.slice_pending=false;
}
void ClientStateReceiver::Close() {
  auto &s=*impl_;s.Thread();
  for(auto &slot:s.work)if(slot.batch) {
    if(!slot.delivered){
      // 邮箱尚未接管的票据没有原生借用。
      s.shadow.CompleteApplication(slot.batch->ticket,false,true,s.events);slot={};
    }else{
      slot.batch->cancelled.store(true,std::memory_order_release);
      slot.batch->resolution.store(ClientStateApplyResolution::Drain,std::memory_order_release);
    }
  }
  for(auto &work:s.resources)if(work&&!work->delivered){
    s.tables.CompleteResourcePreparation(work->job.ticket,ClientResourceState::Unprepared);work.reset();
  }
  s.active=false;
  s.mod_schema.reset();s.mod_shadow.reset();
  s.pending.Clear();s.waiting={};s.next_world.reset();s.body_waiting.reset();s.accepted_plans={};
  s.preload.clear();s.preload_cursor=0;s.shadow.Close();s.tables.Close();
  // 在途回放凭据在Complete／Drain前还要消费；不能用Close提前清掉账本。
  if(!s.Occupied(3))s.events.Close();
  s.Collect();
}
bool ClientStateReceiver::Drained() const {const auto &s=*impl_;s.Thread();return !s.active&&!s.Occupied(3)&&!s.ResourcesInFlight()&&s.retired.empty()&&s.shadow.Drained()&&s.tables.Drained();}
void ClientStateReceiver::Receive(StateMessageKind kind,StateLane lane,std::span<const std::byte> bytes,
    std::uint64_t sequence,std::uint64_t now,bool defer_waiting) {
  auto &s=*impl_;s.Thread();if(!s.active)Fail(StateReadiness::Lifecycle,"client_receive_inactive");
  if(s.uncertain_event)return;
  if(kind==StateMessageKind::Resources) {
    if(lane!=StateLane::Context)Fail(StateReadiness::InvalidEncoding,"client_receive_table_lane");
    if(sequence<=s.last_table_sequence)return;
    const auto result=s.tables.InstallResources(s.route,bytes);
    if(result==ClientTableInstall::Stale)return;
    s.last_table_sequence=sequence;
    if(result==ClientTableInstall::Installed)s.QueueResourcePreload();
  }else if(kind==StateMessageKind::Frame){
    try{s.Store(lane,bytes,now);}
    catch(const Gap &e){s.InvalidFrame(lane==StateLane::Body?2:1,e.what());}
    catch(const ClientShadowError &e){
      if(e.Readiness()!=StateReadiness::InvalidEncoding&&e.Readiness()!=StateReadiness::InvalidReference)throw;
      s.InvalidFrame(lane==StateLane::Body?2:1,e.what());
    }
  }else Fail(StateReadiness::InvalidEncoding,"client_receive_kind");
  Pump(now,!defer_waiting);
}
void ClientStateReceiver::Pump(std::uint64_t now,bool allow_waiting) {
  auto &s=*impl_;s.Thread();s.Collect();if(!s.active)return;
  if(s.uncertain_event){if(!s.Occupied(3))Fail(StateReadiness::Lifecycle,"client_receive_uncertain_playback_stopped");return;}
  if(allow_waiting)s.slice_pending=false;
  const auto expired=s.pending.Expire(now);
  if(expired)s.memory->ApplyMetrics().Wait(expired,ClientApplyWait::Expired);
  for(unsigned i=0;i<2;++i)if(s.waiting[i]&&!s.pending.At(i)){
    if(i==1){s.Recover(3,StateResyncReason::Dependency,true);break;}
    s.waiting[i].reset();
  }
  for(auto &slot:s.work)if(slot.batch&&!slot.delivered&&!slot.awaiting_drain)slot.delivered=s.delivery.Apply(slot.batch);
  s.DeliverResources();
  if(allow_waiting){
    try{s.TryBody();}
    catch(const ClientShadowError &e){
      if(e.Readiness()!=StateReadiness::InvalidEncoding&&e.Readiness()!=StateReadiness::InvalidReference)throw;
      s.body_waiting.reset();s.InvalidFrame(2,e.what());
    }
    // 派发尝试之后才判过期；只收包不派发的轮次不丢等待中的 Body。
    s.ExpireBody();
  }
  // 有界阶段之间返回网络循环，下一轮可先消费回执和新 Body。
  const auto slice=std::chrono::steady_clock::now()+std::chrono::milliseconds(2);
  const auto time_left=[&]{return allow_waiting&&std::chrono::steady_clock::now()<slice;};
  for(const auto i:{1u,0u})if(time_left()&&s.waiting[i]&&!s.waiting[i]->candidate) {
    auto &slot=*s.waiting[i];
#ifdef ISAACCS_E2E
    s.ResumeSlice(slot);
#endif
    const auto streams=StateTargetStreams(slot.target);
    std::optional<ClientShadowBuildResult> decoded;
    try {
      decoded=s.Decode(slot.lane,slot.bytes,streams,slot.decoding,slice);
      if(decoded&&decoded->candidate){
        s.ReceiveCandidate(decoded->candidate,slot.received_us);
        slot.mods=s.mod_shadow;
      }
    }catch(const ClientShadowError &e){
      if(e.Readiness()==StateReadiness::EventGap){
        s.Recover(streams,StateResyncReason::Event);continue;
      }
      if(e.Readiness()==StateReadiness::Budget){
        s.Recover(streams,StateResyncReason::Budget);continue;
      }
      if(e.Readiness()!=StateReadiness::InvalidEncoding&&e.Readiness()!=StateReadiness::InvalidReference)throw;
      s.pending.Remove(i);s.waiting[i].reset();s.InvalidFrame(streams,e.what());continue;
    }
    if(!decoded){
      s.slice_pending=true;
#ifdef ISAACCS_E2E
      if(slot.decoding)slot.slice_end_us=ClientApplyMetrics::Now();
#endif
      continue;
    }
    const auto &result=*decoded;
    if(result.candidate){
      auto &waiting=*s.waiting[i];
      waiting.candidate=result.candidate;waiting.target=result.candidate->Target();
      s.pending.UseDecodedBytes(i,waiting.target.encoded);
      // 比较凭据改借已封存正文，两份完整报文无需同时常驻等待位。
      ClientStateVector<std::byte>{waiting.bytes.get_allocator()}.swap(waiting.bytes);
    }
    else if(result.decision.action==StateAcceptAction::Resync)s.Recover(result.decision.streams,result.decision.reason);
    else if(result.decision.action==StateAcceptAction::Terminal){
      s.pending.Remove(i);s.waiting[i].reset();s.InvalidFrame(streams,"client_receive_conflicting_target");
    }
    else if(result.decision.action==StateAcceptAction::Drop||result.decision.action==StateAcceptAction::Reack){
      if(result.decision.action==StateAcceptAction::Reack)s.Reack(&slot.target);
      s.pending.Remove(i);s.waiting[i].reset();
    }
  }
  if(s.next_world&&(!s.waiting[0]||!s.waiting[0]->decoding)){
    auto latest=std::move(*s.next_world);s.next_world.reset();
    if(std::chrono::steady_clock::now()<latest.deadline&&s.pending.Offer(latest.target,now)){
      s.waiting[0].emplace(std::move(latest));s.slice_pending=true;
    }
  }
  for(const auto i:{1u,0u})if(time_left())s.Try(i,slice);
  if(allow_waiting)s.PumpResourcePreload();
  if(allow_waiting&&!time_left())for(auto &slot:s.waiting)if(slot&&slot->candidate&&!s.Blocked(slot->target)&&
      (!slot->preparing||!slot->preparing->target.facts||!s.ResourcesPending(slot->preparing->target.facts->resources))){
    s.slice_pending=true;
#ifdef ISAACCS_E2E
    if(!slot->slice_end_us)slot->slice_end_us=ClientApplyMetrics::Now();
#endif
  }
  if(s.flat_recovery){
    const auto streams=s.flat_recovery;s.flat_recovery=0;
    s.Recover(streams,StateResyncReason::Dependency);
  }
  if(now>=s.ack_due){s.Reack();s.ack_due=now+StateBudgets().ack_repeat_ms;}
  if(s.recovery_requested&&now>=s.resync_due)if(const auto request=s.shadow.RecoveryRequest()) {
    std::array<std::byte,128> bytes{};if(s.delivery.Control(StateMessageKind::Resync,EncodeStateResyncInto(*request,bytes)))s.resync_due=now+StateBudgets().resync_repeat_ms;
  }
}
bool ClientStateReceiver::HasRunnableSlice() const {
  // 完成回执可能在轮询 UDP 前刚腾出 World 槽，有待转入的报文即可续做。
  impl_->Thread();return impl_->active&&!impl_->uncertain_event&&impl_->slice_pending;
}
void ClientStateReceiver::Complete(const ClientStateApplyCompletion &completion) {
  auto &s=*impl_;s.Thread();
  const auto found=std::find_if(s.work.begin(),s.work.end(),[&](const auto &slot){
    return slot.batch&&SameTicket(slot.batch->ticket,completion.ticket);
  });
  if(found==s.work.end())return;
  auto &slot=*found;
  const auto streams=StateTargetStreams(slot.batch->target.candidate->Target());
  auto &metrics=s.memory->ApplyMetrics();
  if(!slot.awaiting_drain&&completion.reported_us)metrics.Mark(streams,ClientApplyStage::Feedback,
    ClientApplyMetrics::Now()-completion.reported_us);
  const auto &expected=slot.batch->target.events;
  if(completion.replayed_count>completion.replayed.size()||completion.replayed_count>expected.size()||
     (!completion.applied&&completion.drained&&!slot.awaiting_drain&&(completion.replayed_count||completion.uncertain_event))||
     (completion.applied&&(completion.replayed_count!=expected.size()||completion.uncertain_event)))
    Fail(StateReadiness::InvalidEncoding,"client_receive_invalid_playback_prefix");
  for(unsigned i=0;i<completion.replayed_count;++i)if(completion.replayed[i]!=expected[i].value.sequence)
    Fail(StateReadiness::InvalidEncoding,"client_receive_foreign_playback_prefix");
  if(completion.uncertain_event&&(completion.replayed_count>=expected.size()||expected[completion.replayed_count].value.sequence!=completion.uncertain_event))
    Fail(StateReadiness::InvalidEncoding,"client_receive_foreign_uncertain_event");
  if(slot.awaiting_drain&&(completion.replayed_count!=slot.receipt_count||completion.uncertain_event!=slot.receipt_uncertain||completion.applied!=slot.receipt_applied))
    Fail(StateReadiness::InvalidEncoding,"client_receive_drain_receipt_changed");
  if(!s.events.Replayed(std::span(completion.replayed).first(completion.replayed_count)))
    Fail(StateReadiness::InvalidEncoding,"client_receive_playback_receipt");
  if(completion.uncertain_event)s.uncertain_event=true;
  if(slot.awaiting_drain) {
    if(completion.drained&&s.shadow.CompleteDrain(completion.ticket))slot={};
    if(!s.active&&!s.Occupied(3))s.events.Close();
    return;
  }
  slot.receipt_count=completion.replayed_count;slot.receipt_uncertain=completion.uncertain_event;slot.receipt_applied=completion.applied;
  if(s.shadow.CompleteApplication(completion.ticket,completion.applied,completion.drained,s.events)) {
    metrics.Mark(streams,ClientApplyStage::Accept);
    for(unsigned i=0;i<2;++i)if(streams&(1u<<i))s.accepted_plans[i]=slot.batch->target.install;
    // 有序回放和原生提交均已返回，单流批次可以交还对侧的准备租约。
    if(streams!=3)slot.batch->target.effective[streams==1?1:0].reset();
    slot.batch->resolution.store(ClientStateApplyResolution::Committed,std::memory_order_release);
    if(!s.shadow.RecoveryRequest())s.recovery_requested=false;
    slot={};
    if(s.active&&completion.recovery_streams)s.Recover(completion.recovery_streams,StateResyncReason::Apply);
  }else if(completion.drained){
    slot={};
  }else{
    if(s.active)s.Recover(3,StateResyncReason::Apply,true);
    slot.batch->resolution.store(ClientStateApplyResolution::Drain,std::memory_order_release);slot.awaiting_drain=true;
    if(completion.drained&&s.shadow.CompleteDrain(completion.ticket))slot={};
  }
  if(!s.active&&!s.Occupied(3))s.events.Close();
}
void ClientStateReceiver::CompleteResource(const ClientResourcePreparationTicket &ticket,ClientResourceState state) {
  auto &s=*impl_;s.Thread();
  const auto slot=std::find_if(s.resources.begin(),s.resources.end(),[&](const auto &work){return work&&work->job.ticket==ticket;});
  if(slot==s.resources.end())return;
  s.tables.CompleteResourcePreparation(ticket,state);
  if(state!=ClientResourceState::Pending)slot->reset();
}
void ClientStateReceiver::RequireRecovery(std::uint8_t streams,StateResyncReason reason,bool structural){impl_->Thread();impl_->Recover(streams,reason,structural);}
std::array<std::uint32_t,2> ClientStateReceiver::Versions() const {impl_->Thread();return impl_->shadow.HistoricalVersions();}
std::uint64_t ClientStateReceiver::Events() const {impl_->Thread();return impl_->shadow.ReceivedEvents();}
} // namespace cs::state
