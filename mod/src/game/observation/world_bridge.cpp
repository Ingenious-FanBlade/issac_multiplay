#include "game/observation/world_bridge.h"
#include "game/observation/binary_entities.h"
#include "core/capture_cost.h"
#include "replica/stream_e2e.h"
#include "replica/stream_resource_e2e.h"
#include "game/observation/world_entity_owner.h"
#include "binding/world_layout.h"
#include "game/common/bridge.h"
#include "game/common/context.h"
#include "game/input/input_a.h"
#include "game/participants/peer.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "world/observe.h"
#include "world/common/frame_read_regions.h"
#include "game/observation/sources/value_source.h"
#include "game/observation/sources/lua_entity.h"
#include <algorithm>
#include <exception>

namespace cs::game {
StreamSharedResourceScope::StreamSharedResourceScope(bool animation) noexcept {
  if(!IsMain()){foreign_=true;BinaryForeignResourceWrite(true);return;}
  if(const auto capture=CurrentWorldRecording();capture&&capture->binary_entities)
    capture->binary_entities->SharedResourceChanged(animation);
}
StreamSharedResourceScope::~StreamSharedResourceScope() noexcept {
  if(foreign_)BinaryForeignResourceWrite(false);
}
// 背景资源重建使旧房间候选失效。构图和装饰隔离由 Backdrop 入口负责。
void BinaryBackdropChanged(void *backdrop,bool current) noexcept {
  if(!current||!backdrop||!IsMain()||ClientBusy())return;
  if(const auto capture=CurrentWorldRecording();capture&&capture->binary_entities)
    capture->binary_entities->Invalidate();
}
WorldJoinEffectSourceScope::WorldJoinEffectSourceScope() noexcept:exceptions_(std::uncaught_exceptions()) {
  capture::Cost cost(capture::CostPart::Control);
  if(!IsMain()){world::ReportEntityReconfigurationFault(world::ObservationFault::WrongThread);return;}
  capture_=CurrentWorldRecording();if(capture_&&capture_->collector)token_=capture_->collector->BeginJoinEffectSource();
}
WorldJoinEffectSourceScope::~WorldJoinEffectSourceScope() noexcept {
  capture::Cost cost(capture::CostPart::Control);
  const bool normal=returned_&&std::uncaught_exceptions()==exceptions_;
  if(!normal)world::ReportEntityReconfigurationFault(world::ObservationFault::NativeException);
  if(token_&&capture_&&capture_->collector)capture_->collector->EndJoinEffectSource(token_,normal);
}
StreamResourceScope::StreamResourceScope(void *object,bool retired,bool constructing,void *copy_source,const char *operation) noexcept {
  if(!object)return;
  if(!IsMain()){foreign_writer_=true;BinaryForeignResourceWrite(true);return;}
  auto current=CurrentWorldRecording();
  capture::Cost cost(capture::CostPart::Control);
  if(current&&current->binary_entities){current->binary_entities->ResourceChanged(world::PackWord(object),operation);
    qualified_writer_=current->binary_entities->Active();exceptions_=std::uncaught_exceptions();}
#ifdef ISAACCS_E2E
  capture_=std::move(current);exceptions_=std::uncaught_exceptions();
  if(!capture_||!capture_->live_consumer){capture_.reset();return;}
  auto &c=*capture_;retired_=retired;constructing_=constructing;address_=world::PackWord(object);
  const auto old=c.stream_resources.find(address_);
  const auto prior=old==c.stream_resources.end()?WorldCapture::StreamResourceIdentity{}:old->second;
  affects_source_=old!=c.stream_resources.end()&&old->second.observed&&!old->second.temporary;
  // 未进入来源闭包的UI/新对象不能使整房样本作废；已经观测的真实owner仍进入即撤资格。
  if(affects_source_){
    E2EResourceChangeFromNative(object,address_,old->second.generation,c.location_window,retired,constructing,copy_source!=nullptr,operation);
    ++c.stream_resource_depth;
  }
  try {
    const auto address=address_;
    if(retired)c.stream_resources.erase(address);
    else {
      if(c.next_stream_resource>=UINT64_MAX-1||(!c.stream_resources.contains(address)&&c.stream_resources.size()>=65536))throw std::bad_alloc();
      const bool temporary=prior.temporary||(constructing&&c.location_window);
      const bool observed=prior.observed&&!constructing;
      std::uint64_t copy_generation=prior.constructing?prior.copy_generation:0;
      std::uint32_t copy_address=prior.constructing?prior.copy_address:0;
      if(copy_source){
        const auto source=world::PackWord(copy_source);
        if(source==address)throw std::bad_alloc();
        auto it=c.stream_resources.find(source);
        if(it==c.stream_resources.end()){
          if(c.stream_resources.size()>=65535)throw std::bad_alloc();
          it=c.stream_resources.emplace(source,WorldCapture::StreamResourceIdentity{++c.next_stream_resource,temporary,false}).first;
        }
        // 按值返回可经临时对象再复制到Lua；只沿已经确认的只读临时copy来源归并。
        auto origin=it;
        for(unsigned depth=0;depth<16&&origin->second.temporary&&origin->second.copy_generation;++depth){
          const auto parent=c.stream_resources.find(origin->second.copy_address);
          if(parent==c.stream_resources.end()||parent==origin||parent->second.generation!=origin->second.copy_generation)break;
          origin=parent;
        }
        if(temporary&&!origin->second.temporary)origin->second.observed=true;
        copy_generation=origin->second.generation;copy_address=origin->first;
        if(temporary)copied_generation_=copy_generation;
      }
      if(c.next_stream_resource==UINT64_MAX)throw std::bad_alloc();
      c.stream_resources[address]={copied_generation_?copied_generation_:++c.next_stream_resource,observed,temporary,
        copy_generation,copy_address,constructing||prior.constructing};
    }
  }catch(...){c.observation.Invalidate("stream_resource_identity_budget");}
#else
  (void)retired;(void)constructing;(void)copy_source;(void)operation;
#endif
}
StreamResourceScope::~StreamResourceScope() noexcept {
  if(foreign_writer_){BinaryForeignResourceWrite(false);return;}
  if(qualified_writer_&&(!returned_||exceptions_!=std::uncaught_exceptions())){
    if(const auto current=CurrentWorldRecording();current&&current->binary_entities)current->binary_entities->Invalidate();
  }
#ifdef ISAACCS_E2E
  if(!capture_)return;auto &c=*capture_;if(affects_source_&&c.stream_resource_depth)--c.stream_resource_depth;
  capture::Cost cost(capture::CostPart::Control);
  // copy原链内部Reset不能把读取副本改成独立世界资源；只在原链正常返回后恢复来源代次。
  if(copied_generation_&&returned_&&exceptions_==std::uncaught_exceptions()){
    if(auto it=c.stream_resources.find(address_);it!=c.stream_resources.end())it->second.generation=copied_generation_;
  }
  if(constructing_)if(auto it=c.stream_resources.find(address_);it!=c.stream_resources.end())it->second.constructing=false;
  // 原生析构内部还会调用Reset；外层最终收口再次抹除，不能留下嵌套Reset分配的新token。
  if(retired_)c.stream_resources.erase(address_);
  if(!returned_||exceptions_!=std::uncaught_exceptions())c.observation.Invalidate("stream_resource_original_exception");
#endif
}
#ifdef ISAACCS_E2E
std::uint64_t StreamResourceGeneration(void *object,bool copied_value) noexcept {
  auto c=CurrentWorldRecording();if(!IsMain()||!c||!c->live_consumer||!object||c->stream_resource_depth)return 0;
  capture::Cost cost(capture::CostPart::Control);
  try {
    const auto address=world::PackWord(object);if(const auto it=c->stream_resources.find(address);it!=c->stream_resources.end()){
      // userdata内嵌ANM由Lua持有，是getter返回值而非世界owner；其析构不得撤销世界。
      if(copied_value){
        if(it->second.copy_generation){
          const auto source=c->stream_resources.find(it->second.copy_address);
          if(source==c->stream_resources.end()||source->second.generation!=it->second.copy_generation)return 0;
          if(!source->second.temporary)source->second.observed=true;
          it->second.generation=it->second.copy_generation;
        }
        it->second.temporary=true;it->second.observed=false;
      }
      if(!it->second.temporary)it->second.observed=true;
      return it->second.generation;
    }
    if(c->next_stream_resource==UINT64_MAX||c->stream_resources.size()>=65536)throw std::bad_alloc();
    const auto next=++c->next_stream_resource;c->stream_resources.emplace(address,WorldCapture::StreamResourceIdentity{next,!copied_value,copied_value});return next;
  }catch(...){c->observation.Invalidate("stream_resource_identity_budget");return 0;}
}
#endif
WorldEntityReconfigurationScope::WorldEntityReconfigurationScope(void *object,world::HookCounters counters) noexcept
    :counters_(counters),exceptions_(std::uncaught_exceptions()) {
  capture::Cost cost(capture::CostPart::Control);
  if(counters_.calls)counters_.calls->fetch_add(1,std::memory_order_relaxed);
  if(!IsMain()){
    if(counters_.foreign)counters_.foreign->fetch_add(1,std::memory_order_relaxed);
    world::ReportEntityReconfigurationFault(world::ObservationFault::WrongThread);return;
  }
  capture_=CurrentWorldRecording();
  if(capture_&&capture_->collector)token_=capture_->collector->BeginDeliriumReconfiguration(world::PackWord(object));
}
WorldEntityReconfigurationScope::~WorldEntityReconfigurationScope() noexcept {
  capture::Cost cost(capture::CostPart::Control);
  const bool normal=returned_&&std::uncaught_exceptions()==exceptions_;
  if(!normal)world::ReportEntityReconfigurationFault(world::ObservationFault::NativeException);
  if(token_&&capture_&&capture_->collector)capture_->collector->EndDeliriumReconfiguration(token_,normal);
}
void WorldEntityReconfigurationScope::Returned() noexcept {
  returned_=true;if(counters_.completed)counters_.completed->fetch_add(1,std::memory_order_relaxed);
}
WorldPlayerLifeSourceScope::WorldPlayerLifeSourceScope(void *player,bool ghost) noexcept {
  if(!IsMain()||!HostBusy()||ClientBusy())return;
  capture_=CurrentWorldRecording();
  if(capture_&&capture_->collector)witness_=capture_->collector->BeginPlayerLife(world::PackWord(player),ghost);
}
void WorldPlayerLifeSourceScope::Returned() noexcept {
  if(witness_.address&&capture_&&capture_->collector)capture_->collector->CompletePlayerLife(witness_);
}
WorldPlayerChildSourceScope::WorldPlayerChildSourceScope(void *object,unsigned kind,world::HookCounters counters,unsigned slot) noexcept
    :counters_(counters),exceptions_(std::uncaught_exceptions()) {
  capture::Cost cost(capture::CostPart::Control);
  if(counters_.calls)counters_.calls->fetch_add(1,std::memory_order_relaxed);
  if(!IsMain()){if(counters_.foreign)counters_.foreign->fetch_add(1,std::memory_order_relaxed);world::ReportEntityReconfigurationFault(world::ObservationFault::WrongThread);return;}
  capture_=CurrentWorldRecording();
#ifdef ISAACCS_E2E
  if(kind==4&&capture_&&Settings.diagnostics){const auto *id=capture_->identities.Find(world::ObjectDomain::Entity,world::PackWord(object));
    Trace("world_esau_source_begin","local=%llu readiness=%u current_public=%u slot=%u",static_cast<unsigned long long>(id?id->local_id:0),id?id->readiness:0,unsigned(id&&id->current_public_ready),slot);}
#endif
  if(capture_&&capture_->collector)token_=capture_->collector->BeginPlayerChildSource(world::PackWord(object),kind,slot);
}
WorldPlayerChildSourceScope::~WorldPlayerChildSourceScope() noexcept {
  capture::Cost cost(capture::CostPart::Control);
  const bool normal=returned_&&std::uncaught_exceptions()==exceptions_;
  if(!normal)world::ReportEntityReconfigurationFault(world::ObservationFault::NativeException);
  if(token_&&capture_&&capture_->collector)capture_->collector->EndPlayerChildSource(token_,normal,body_reached_);
}
void WorldPlayerChildSourceScope::Returned(bool body_reached) noexcept {
  returned_=true;body_reached_=body_reached;if(counters_.completed)counters_.completed->fetch_add(1,std::memory_order_relaxed);
}
// 构造／析构均在BinaryEntities完整定义处生成，其他使用方不实例化其不完整类型清理代码。
WorldCapture::WorldCapture()=default;
WorldCapture::~WorldCapture() {
  if(reserve_charged) ledger.Release(world::kObservationReserveBytes);
  if (faults.unreadable) {
    VirtualFree(reinterpret_cast<void *>(uintptr_t(faults.unreadable)), 0,
                MEM_RELEASE);
    ledger.Release(65536);
  }
}
namespace {
std::shared_ptr<WorldRecording> recording;
world::ObservationFailureCounts observed_faults{};
ValueSourceFailures observed_value_faults{};
core::TickPostState prepared_post{};
bool frame_prepared = false;
bool activation_pending = false;
world::Collector *Collector() noexcept {
  return IsMain() && recording ? recording->collector.get() : nullptr;
}
// 帧首与提交前共用实际故障分类；不把线程故障和异常展开合并为同一原因。
// 两类同时发生时异常展开优先，trace仍保留两项增量，collector保留既有首因。
void ObserveFailures(world::Collector &collector) noexcept {
  const auto faults = world::ObservationFailures();
  const bool thread = faults.wrong_thread != observed_faults.wrong_thread;
  const bool exception =
      faults.native_exception != observed_faults.native_exception;
  if (collector.Participating() && (thread || exception)) {
    Trace("world_observation_fault",
          "wrong_thread_delta=%llu native_exception_delta=%llu",
          faults.wrong_thread - observed_faults.wrong_thread,
          faults.native_exception - observed_faults.native_exception);
    collector.Fault(exception ? world::Reason::UnpairedScope
                              : world::Reason::WrongThread,
                    exception ? "observation_native_exception"
                              : "observation_wrong_thread");
  }
  observed_faults = faults;
  const auto values=ObservationValueSourceFailures();
  if(collector.Participating()&&(values.wrong_thread!=observed_value_faults.wrong_thread||values.exception!=observed_value_faults.exception||values.budget!=observed_value_faults.budget)) {
    const auto reason=values.exception!=observed_value_faults.exception?world::Reason::UnpairedScope:values.wrong_thread!=observed_value_faults.wrong_thread?world::Reason::WrongThread:world::Reason::Budget;
    if(recording) recording->observation.Invalidate("native_actual_value_source_fault");
    collector.Fault(reason,"native_actual_value_source");
  }
  observed_value_faults=values;
}
std::uint32_t Enter(world::Observation kind, std::uint32_t object,
                    const std::uint32_t *args, std::uint32_t count,
                    bool native) noexcept {
  capture::Cost cost(capture::CostPart::Control);
  auto *collector = Collector();
  return collector
             ? (native ? collector->NativeEnter(kind, object, args, count)
                       : collector->ObserveEnter(kind, object, args, count))
             : 0;
}
void Leave(std::uint32_t call, std::uint32_t result, bool normal,
           bool native) noexcept {
  capture::Cost cost(capture::CostPart::Control);
  if (auto *collector = Collector()) {
    if (native)
      collector->NativeLeave(call, result, normal);
    else
      collector->ObserveLeave(call, result, normal);
  }
}
// 生命周期观察在world关闭时也撤销输入资格；仅比较当前已知地址，不读取构造中的对象。
std::uint32_t ControlEnter(world::Observation kind, std::uint32_t object,
                           const std::uint32_t *args,
                           std::uint32_t count) noexcept {
  capture::Cost cost(capture::CostPart::Control);
  // ObservationScope先做control_enter再做enter；只读证言必须先于WorldRevokePlayer，输入仍原样撤销。
  if(kind==world::Observation::Replace&&args&&count==2)if(auto *collector=Collector()){
    const auto basis=collector->BeforePlayerReplaceSource(object,args[0],args[1]);
    if(Settings.diagnostics&&!Settings.world_e2e_token.empty())Trace("world_player_replace_state","before_control_basis=%u",basis);
  }
  auto *r = Runtime();
  if (!r || !r->core_started || r->finished || r->retiring || r->join_ordinal)
    return 0;
  std::uint32_t mask = 0;
  for (unsigned i = 0; i < r->session.ParticipantLimit(); ++i) {
    if (!r->session.HasParticipant(i) || (i && r->Remote(i).retiring))
      continue;
    const auto owns = [&](std::uint32_t value) noexcept {
      void *player = reinterpret_cast<void *>(uintptr_t(value));
      return value && (r->controlled_player[i] == player ||
                       r->topology[i].Contains(player, false));
    };
    bool changed = owns(object);
    if (kind == world::Observation::Replace && args)
      for (unsigned a = 0; a < count && a < 2; ++a)
        changed |= owns(args[a]);
    if (!changed)
      continue;
    mask |= 1u << i;
    r->binding_defer_through[i] =
        (std::max)(r->binding_defer_through[i], r->loop);
    WorldRevokePlayer(i);
    if (r->players[i].BoundValid())
      r->players[i].Invalidate();
    if (i) {
      r->Remote(i).life.pending_reason = "native_player_lifecycle";
      DiscardPeerWindow(*r, i);
    } else
      DiscardInputEdges();
    Trace("control_lifecycle",
          "ordinal=%u event=enter observation=%u generation=%llu loop=%llu", i,
          unsigned(kind), r->players[i].Generation(), r->loop);
  }
  if (mask) {
    ++r->control_lifecycle_depth;
    ++r->query_epoch;
  }
  return mask;
}
void ControlLeave(std::uint32_t mask, bool normal) noexcept {
  capture::Cost cost(capture::CostPart::Control);
  auto *r = Runtime();
  if (!r || !mask)
    return;
  if (r->control_lifecycle_depth)
    --r->control_lifecycle_depth;
  else {
    FailRuntime("control_lifecycle_scope_mismatch");
    return;
  }
  if (r->finished || normal)
    return;
  for (unsigned i = 0; i < r->session.ParticipantLimit(); ++i)
    if (mask & (1u << i)) {
      r->control_quarantined[i] = true;
      if (i)
        RequestRemoteRetirement(*r, i, core::SessionCause::PeerProtocol);
      Trace("control_lifecycle",
            "ordinal=%u event=quarantined reason=native_exception loop=%llu", i,
            r->loop);
    }
}
// 原生成功才转移根来源；这里不读对象、不授予输入，后续稳定入口重新验证完整图。
void ControlReplace(std::uint32_t old_value, std::uint32_t new_value) noexcept {
  capture::Cost cost(capture::CostPart::Control);
  auto *r = Runtime();
  if (!r || r->finished || !old_value || !new_value)
    return;
  void *old_player = reinterpret_cast<void *>(uintptr_t(old_value));
  void *replacement = reinterpret_cast<void *>(uintptr_t(new_value));
  for (unsigned i = 0; i < r->session.ParticipantLimit(); ++i)
    if (r->session.HasParticipant(i) && r->controlled_player[i] == old_player) {
      r->controlled_player[i] = replacement;
      Trace("control_lifecycle",
            "ordinal=%u event=replace_root old=%p replacement=%p loop=%llu", i,
            old_player, replacement, r->loop);
    }
}
const world::ObservationSink sink{Enter,        Leave,       ControlEnter,
                                  ControlLeave, ForeignCall, ControlReplace};
world::Stage StageFor(WorldBoundary kind) noexcept {
  return kind == WorldBoundary::Owner  ? world::Stage::Owner
         : kind == WorldBoundary::Room ? world::Stage::Room
                                       : world::Stage::Tail;
}
} // namespace
bool WorldCaptureActive() noexcept { return IsMain()&&recording&&recording->collector&&recording->collector->Participating(); }
void InitializeWorldBridge() noexcept {
  world::InitializeFrameReadRegions([]() noexcept {Trace("read_regions_guard","cross_call_cache_disabled");});
  world::SetObservationSink(&sink);
}
world::MemoryReader WorldMemoryReader() noexcept {
  world::MemoryReader result{};
  if (!IsMain() || !Base)
    return result;
  IMAGE_DOS_HEADER dos{};
  IMAGE_NT_HEADERS32 nt{};
  if (!ReadMemory(reinterpret_cast<void *>(Base), &dos, sizeof(dos)) ||
      dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 ||
      !ReadAt(reinterpret_cast<void *>(Base),
              static_cast<std::size_t>(dos.e_lfanew), nt) ||
      nt.Signature != IMAGE_NT_SIGNATURE ||
      nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    return result;
  result.copy = [](std::uint32_t address, void *out,
                   std::size_t size) noexcept {
    return ReadMemory(reinterpret_cast<void *>(uintptr_t(address)), out, size);
  };
  result.image_base = static_cast<std::uint32_t>(Base);
  result.image_size = nt.OptionalHeader.SizeOfImage;
  result.main_thread = MainThread;
  result.copy_image = [](std::uint32_t address, void *out, std::size_t size) noexcept {
    SIZE_T copied=0;
    return ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<const void *>(uintptr_t(address)),out,size,&copied) && copied==size;
  };
  result.roots=[](std::uint32_t image_base,const std::array<std::uint32_t,4> &offsets,
      std::array<std::uint32_t,4> &values) noexcept {
    if(!IsMain())return false;
    for(unsigned i=0;i<values.size();++i){
      const auto base=i?values[0]:image_base;
      const auto address=std::uint64_t(base)+offsets[i];
      if(!base||!world::CheckedNativeCopy(address,&values[i],sizeof(values[i]),world::FrameRegionReader::Roots))return false;
    }
    return true;
  };
  result.classify=[](world::ObjectDomain domain,std::uint32_t address,std::uint32_t image_base,
      std::uint32_t image_size,std::uint32_t &vptr,bool &base_known) noexcept {
    base_known=false;if(!IsMain())return world::Class::Unknown;
    // RTTI 仍逐项读取并定界。与其他读者共享页权限，分类结果不缓存。
    world::NativeSource source;
    source.image_base=image_base;source.image_size=image_size;
    source.copy=[](void *,std::uint64_t p,void *out,std::size_t size) noexcept {
      return world::CheckedNativeCopy(p,out,size,world::FrameRegionReader::Classify);
    };
    return world::ClassifyNative(source,domain,address,vptr,base_known);
  };
  return result;
}
std::shared_ptr<WorldRecording> CurrentWorldRecording() noexcept {
  return IsMain() ? recording : nullptr;
}
bool AttachPreparedWorld(std::shared_ptr<WorldRecording> next) noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager ||
      !Settings.enabled || !Capabilities[size_t(Group::World)] ||
      !Capabilities[size_t(Group::Loop)] ||
      !Capabilities[size_t(Group::Player)] || !next || !next->collector ||
      !next->storage.Ready() || !next->identities.Ready())
    return false;
  bool observations[world::kObservationCount]{};
  world::layout::ObservationMask(observations, world::kObservationCount);
  for (const auto registered : observations)
    if (!registered)
      return false;
  if (recording &&
      (!recording->collector->Drained() || !recording->storage.Closed()))
    return false;
  recording = std::move(next);
  recording->ConnectSources();
  // 命令在 FrameBegin 内执行时，world 的前半个入口已过去；激活延后至下一个
  // Manager，不能以半个窗口补造完整阶段或把同帧出生归到 run=0。
  activation_pending = true;
  observed_faults = world::ObservationFailures();
  observed_value_faults=ObservationValueSourceFailures();
  prepared_post = {};
  frame_prepared = false;
  return true;
}
void StopWorldRecording() noexcept {
  ArmObservationValueSources(false);
  if (IsMain())
    activation_pending = false;
  if (auto *collector = Collector()) {
    if(recording->binary_entities)recording->binary_entities->Active(false);
    recording->observation.RequestStop();
    collector->Stop();
  }
  if(IsMain())capture::FrameEnd();
}
std::array<std::uint64_t,4> CurrentLiveSource() noexcept {
  if(!IsMain()||!recording||!recording->live_consumer||!recording->observation.Active()||
     !recording->collector||!recording->collector->Participating()||!RunActive())return {};
  const auto run=recording->collector->CurrentRunToken(),room=recording->collector->CurrentRoomToken();
  if(!run||!room)return {};
  return {recording->session_key,recording->epoch,run,room};
}
bool DetachClosedWorld(const std::shared_ptr<WorldRecording> &closed) noexcept {
  if (!IsMain() || recording != closed || !closed || !closed->collector ||
      !closed->collector->Drained() || !closed->storage.Closed())
    return false;
  recording.reset();
  activation_pending = false;
  prepared_post = {};
  frame_prepared = false;
  return true;
}
void WorldFrameBegin(std::uint64_t loop) noexcept {
  capture::FrameBegin(loop,recording&&recording->binary_entities&&recording->binary_entities->Active()&&RunActive());
  capture::Cost cost(capture::CostPart::Native);
  auto *collector = Collector();
  if (!collector)
    return;
  if (activation_pending) {
    activation_pending = false;
    recording->observation.ActivateObservation(recording->session_key,recording->epoch);
    if (!collector->Activate())
      collector->Fault(world::Reason::PrepareFailed, "activate_prepared");
  }
  prepared_post = {};
  frame_prepared = false;
  ObserveFailures(*collector);
  if (!collector->Participating()) {
    if(recording->binary_entities)recording->binary_entities->Invalidate();
    recording->live.Invalidate("live_collector_not_participating");
    return;
  }
  collector->BeginOuter(loop, OwnerToken(Owner::Manager), RunActive());
  if(recording->binary_entities)recording->binary_entities->FrameBegin();
  // 原生run/room撤销即取消活动上下文，不等待可选Lua回调或下一次30tick采集。
  auto &location=recording->hierarchy;
  if(location.valid&&(location.run!=collector->CurrentRunToken()||location.room!=collector->CurrentRoomToken())) {
    recording->live.Invalidate("live_run_or_room_changed");
    location.Invalidate(location.run!=collector->CurrentRunToken());
    recording->latest_room={}; recording->latest_level={}; recording->latest_continuity={};
  }
  WorldRefreshAnchors();
}
world::PlayerAnchor WorldPlayerAnchor(unsigned ordinal, void *player) noexcept {
  capture::Cost cost(capture::CostPart::Native);
  world::PlayerAnchor anchor{};
  auto *collector = Collector();
  if (!collector || !collector->Recording())
    return anchor;
  auto *r = Runtime();
  if (!IsMain() || !r || r->finished || r->retiring || r->failure ||
      !r->session.HasParticipant(ordinal) || r->blocks ||
      !r->players[ordinal].BoundValid() || InBlockedContext())
    return anchor;
  // ST7：B 的不可读/不唯一分支未必递增原绑定代次，因此这里独立复读拓扑。
  const auto topology = ReadTopology(
      ordinal, ordinal ? r->Remote(ordinal).device.Object() : nullptr);
  if (Runtime() != r || Collector() != collector ||
      !topology.known || !topology.unique || !topology.Alive() ||
      topology.room != r->topology[ordinal].room ||
      topology.pair != r->players[ordinal].Bound() ||
      !SameControlGroup(topology, r->topology[ordinal]))
    return anchor;
  if (!player)
    player = topology.pair.player;
  if (!topology.Contains(player))
    return anchor;
  anchor.player = world::PackWord(player);
  anchor.device = world::PackWord(topology.pair.device);
  anchor.key = topology.pair.key;
  anchor.session = SessionGeneration(*r);
  anchor.generation = r->players[ordinal].Generation();
  anchor.valid = true;
  return anchor;
}
std::uint64_t WorldEntityLocalId(void *player) noexcept {
  auto *collector = Collector();
  return collector && collector->Recording()
             ? collector->EntityLocalId(world::PackWord(player))
             : 0;
}
void WorldRefreshAnchors(bool topology_just_read) noexcept {
  capture::Cost cost(capture::CostPart::Native);
  auto *collector = Collector();
  auto *r = Runtime();
  if (!collector || !collector->Recording() || !r)
    return;
  world::PlayerAnchorSet values{};
  if (!r->finished && !r->retiring && !r->failure && !r->blocks &&
      !InBlockedContext()) {
    unsigned mask = 0;
    TopologyDevices devices{};
    for (unsigned ordinal = 0; ordinal < r->session.ParticipantLimit(); ++ordinal)
      if (r->session.HasParticipant(ordinal) && r->players[ordinal].BoundValid()) {
        mask |= 1u << ordinal;
        if (ordinal)
          devices[ordinal] = r->Remote(ordinal).device.Object();
      }
    // 只有生命周期刚读到的见证可直接消费。每个其他 Hook 同步边界独立取公共快照。
    const auto project = [&](const TopologyGroups &groups) noexcept {
      const auto session = SessionGeneration(*r);
      for (unsigned ordinal = 0; ordinal < groups.size(); ++ordinal) {
        if (!(mask & (1u << ordinal)))
          continue;
        const auto &group = groups[ordinal];
        const auto &binding = r->players[ordinal];
        if (!group.known || !group.unique || !group.Alive() ||
            group.room != r->topology[ordinal].room ||
            group.pair != binding.Bound() ||
            !SameControlGroup(group, r->topology[ordinal]))
          continue;
        for (unsigned b = 0; b < group.body_count; ++b)
          if (group.bodies[b].CanConsume())
            values[ordinal * kMaximumGroupBodies + b] = {
                world::PackWord(group.bodies[b].player),
                world::PackWord(group.pair.device), group.pair.key,
                session, binding.Generation(), true};
      }
    };
    if (topology_just_read) {
      project(r->topology);
    } else if (mask) {
      TopologyGroups refreshed;
      ReadTopologies(mask, devices, refreshed);
      if (Runtime() != r || Collector() != collector)
        return;
      project(refreshed);
    }
  }
  capture::Cost install(capture::CostPart::AnchorInstall);
  capture::CountAnchorChanges(collector->ReplacePlayerAnchors(values,
      [](std::uint32_t address) noexcept {
        if (recording->binary_entities)
          recording->binary_entities->RetireBody(address);
      },
      [](unsigned slot, const world::PlayerAnchor &before,
         const world::PlayerAnchor &after) noexcept {
        ObservationControlChange(slot, before, after, "observed_topology_or_lifecycle_refresh");
      }));
}
// 身体组成员与当前输入锚点分开：死亡／ghost可以仍在已复核的唯一组内，但不获得消费资格。
// 只读本普通窗口的缓存拓扑，不增加逐consumer原生扫描；多组重叠、未知或跨房直接保留缺口。
void ObservationPlayerMembership(void *object,ObservationSubjectValue &value) noexcept {
  auto c=CurrentWorldRecording();auto *r=Runtime();
  if(!IsMain()||!c||!c->location_window||!r||r->finished||r->retiring||r->failure||r->blocks)return;
  unsigned found_ordinal=core::kMaximumParticipants,found_body=0;
  for(unsigned ordinal=0;ordinal<r->session.ParticipantLimit();++ordinal){
    if(!r->session.HasParticipant(ordinal))continue;
    const auto &topology=r->topology[ordinal];
    if(!topology.run||!topology.known||!topology.unique||topology.fault!=TopologyFault::None||!(topology.room==r->topology[0].room))continue;
    if(r->players[ordinal].BoundValid()&&topology.pair!=r->players[ordinal].Bound())continue;
    for(unsigned i=0;i<topology.body_count;++i)if(topology.bodies[i].player==object){
      if(found_ordinal!=core::kMaximumParticipants)return;
      found_ordinal=ordinal;found_body=i;
    }
  }
  if(found_ordinal==core::kMaximumParticipants||(value.ordinal<4&&value.ordinal!=found_ordinal))return;
  const auto ordinal=found_ordinal;const auto session=SessionGeneration(*r);
  if(c->control_sessions[ordinal]!=session){
    c->control_sessions[ordinal]=session;
    c->participant_keys[ordinal]=c->observation.AllocateRelatedSubject(world::observation::IdentitySpace::Participant);
    c->control_group_keys[ordinal]=c->observation.AllocateRelatedSubject(world::observation::IdentitySpace::ControlGroup);
  }
  const auto &body=r->topology[ordinal].bodies[found_body];
  value.ordinal=ordinal;value.participant=c->participant_keys[ordinal].local;value.group=c->control_group_keys[ordinal].local;
  value.control_session=session;value.binding_revision=r->players[ordinal].Generation();value.device_revision=c->device_relation_revisions[ordinal];
  value.group_known=true;value.body_role=unsigned(body.role);value.body_active=body.active;
  value.can_consume=r->players[ordinal].BoundValid()&&body.CanConsume();
}
void WorldRevokePlayer(unsigned ordinal) noexcept {
  capture::Cost cost(capture::CostPart::Control);
  if (ordinal >= core::kMaximumParticipants)
    return;
  if (auto *collector = Collector())
    for (unsigned b = 0; b < kMaximumGroupBodies; ++b) {
      const auto slot=ordinal*kMaximumGroupBodies+b;
      const auto before=collector->CurrentAnchor(slot);
      if(recording&&recording->binary_entities)recording->binary_entities->RetireBody(before.player);
      collector->RevokePlayer(slot);
      if(before.valid) ObservationControlChange(slot,before,{},"explicit_binding_revocation");
    }
}
void WorldPlayerReturned(unsigned ordinal, const world::PlayerAnchor &before,
                         std::uint64_t tick, bool normal, bool body) noexcept {
  capture::Cost cost(capture::CostPart::Native);
  auto *collector = Collector();
  auto *r = Runtime();
  if (!collector || !r || ordinal >= core::kMaximumParticipants)
    return;
  void *player = reinterpret_cast<void *>(uintptr_t(before.player));
  const auto after = WorldPlayerAnchor(ordinal, player);
  if (Runtime() != r || Collector() != collector)
    return;
  const auto &group = r->topology[ordinal];
  for (unsigned b = 0; b < group.body_count; ++b)
    if (group.bodies[b].player == player) {
      collector->PlayerReturned(
          ordinal * kMaximumGroupBodies + b, before, after,
          r->pipeline.SimulationTick() == tick ? tick : 0, normal, body);
      return;
    }
}
void WorldNativeReturned(const core::TickBoundary &boundary,
                         std::uint64_t tick) noexcept {
  capture::Cost cost(capture::CostPart::Native);
  if (auto *collector = Collector()) {
    WorldRefreshAnchors();
    if (Collector() != collector)
      return;
    collector->NativeReturned({boundary.loop, OwnerToken(Owner::Manager), tick,
                               boundary.scope_token,
                               boundary.kind == core::TickKind::StandardRoom
                                   ? world::AuthorityKind::StandardRoom
                                   : world::AuthorityKind::OddPlayer});
  }
}
void WorldPreparePostState(bool normal) noexcept {
  capture::Cost cost(capture::CostPart::Native);
  if (auto *collector = Collector()) {
    if (!collector->Participating()) {
      if(recording->binary_entities)recording->binary_entities->Invalidate();
      prepared_post = {};
      frame_prepared = true;
      return;
    }
    WorldRefreshAnchors();
    if (Collector() != collector)
      return;
    ObserveFailures(*collector);
    if(collector->LegacyDiagnostics()){
      // 分开计量旧原生批次与后续二进制来源，定位主线程采集成本。
      capture::Cost legacy(capture::CostPart::Copy);
      prepared_post = collector->PreparePostState(normal);
    }else prepared_post=collector->PreparePostState(normal);
    const auto *runtime=Runtime();
    const auto completed_tick=runtime&&!runtime->retiring&&!runtime->finished&&!runtime->failure?runtime->clock.Completed():0;
    const bool binary_prepared=recording->binary_entities&&recording->binary_entities->Prepare(normal,completed_tick);
    if(binary_prepared)recording->binary_entities->CommitTransition();
    if(prepared_post.status!=core::PostStateStatus::Disabled)prepared_post=collector->NativePrepared(binary_prepared);
    frame_prepared = true;
    // 无 Authority 的 gap 已由采集器发布；没有 pending 世界值需要 core 提交。
  }
}
core::TickPostState WorldPostState(std::uint64_t tick) noexcept {
  auto *collector = Collector();
  if (!collector)
    return {};
  if (!collector->Participating())
    return {};
  if (frame_prepared && prepared_post.status == core::PostStateStatus::Disabled)
    return {};
  if (frame_prepared && prepared_post.tick == tick)
    return prepared_post;
  // 错配只停 world，真实 tick 仍可走 FinishPostState。
  collector->Fault(world::Reason::UnpairedScope, "post_candidate_mismatch");
  core::TickPostState post{};
  post.status = core::PostStateStatus::Unavailable;
  post.tick = tick;
  post.reason = static_cast<std::uint32_t>(world::Reason::UnpairedScope);
  return post;
}
void WorldPublishPostState(bool committed,
                           const core::TickPostState &post) noexcept {
  capture::Cost cost(capture::CostPart::Handoff);
  // 在Collector关闭本outer前提交独立二进制源；不受旧Lua／JSON资源路径成败影响。
  if(recording&&recording->binary_entities)recording->binary_entities->Commit(committed,post);
  if (auto *collector = Collector())
    collector->PublishPostState(committed, post);

}
void WorldFrameEnd() noexcept {
  {
  capture::Cost cost(capture::CostPart::Control);
  if(recording&&recording->binary_entities)recording->binary_entities->FrameEnd();
  if (auto *collector = Collector())
    collector->EndOuter();
  }
  capture::FrameEnd();
}

BinaryFloorScope::BinaryFloorScope() noexcept:exceptions_(std::uncaught_exceptions()) {
  if(!IsMain() || !recording || !recording->binary_entities)return;
  active_=true;recording->binary_entities->FloorBoundary(true,false);
}
BinaryFloorScope::~BinaryFloorScope() noexcept {
  if(active_ && IsMain() && recording && recording->binary_entities)
    recording->binary_entities->FloorBoundary(false,exceptions_==std::uncaught_exceptions());
}
WorldBoundaryScope::WorldBoundaryScope(WorldBoundary kind, const void *object,
                                       bool enabled) noexcept
    : kind_(kind), exceptions_(std::uncaught_exceptions()) {
  capture::Cost cost(capture::CostPart::Control);
  auto *collector = Collector();
  if (!enabled || !collector || !collector->Recording())
    return;
  if (kind == WorldBoundary::Lifecycle || kind == WorldBoundary::RunEnd ||
      kind == WorldBoundary::RunStart) {
    if (kind == WorldBoundary::RunStart)
      collector->RunBeginning();
    collector->LifecycleEnter(kind == WorldBoundary::RunEnd);
    active_ = true;
    return;
  }
  // 内部 Start/Restore/Door/Join Room 只保留生命周期，不充当普通完整阶段。
  if (!HasOwner(Owner::Manager) || InBlockedContext() ||
      HasOwner(Owner::Join) || HasOwner(Owner::Render))
    return;
  if (TopOwner() != Owner::Game)
    return;
  scope_ = OwnerToken(Owner::Game);
  active_ = scope_ != 0;
  if (active_)
    collector->EnterStage(StageFor(kind), scope_, world::PackWord(object));
}
WorldBoundaryScope::~WorldBoundaryScope() noexcept {
  capture::Cost cost(capture::CostPart::Control);
  auto *collector = Collector();
  if (!active_ || !collector)
    return;
  const bool normal = exceptions_ == std::uncaught_exceptions();
  if (kind_ == WorldBoundary::Lifecycle || kind_ == WorldBoundary::RunEnd ||
      kind_ == WorldBoundary::RunStart)
    collector->LifecycleLeave(normal);
  else
    collector->LeaveStage(StageFor(kind_), scope_, normal);
  if (kind_ == WorldBoundary::RunStart)
    collector->RunOwnerReturned(normal);
}
} // namespace cs::game
