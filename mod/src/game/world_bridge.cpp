#include "game/world_bridge.h"
#include "game/world_entity_owner.h"
#include "binding/world_layout.h"
#include "game/bridge.h"
#include "game/context.h"
#include "game/input_a.h"
#include "game/peer.h"
#include "game/state.h"
#include "game/tick.h"
#include "world/observe.h"
#include "game/world_sources/value_source.h"
#include "game/world_sources/lua_entity.h"
#include <algorithm>
#include <exception>

namespace cs::game {
WorldEntityReconfigurationScope::WorldEntityReconfigurationScope(void *object,world::HookCounters counters) noexcept
    :counters_(counters),exceptions_(std::uncaught_exceptions()) {
  if(counters_.calls)counters_.calls->fetch_add(1,std::memory_order_relaxed);
  if(!IsMain()){
    if(counters_.foreign)counters_.foreign->fetch_add(1,std::memory_order_relaxed);
    world::ReportEntityReconfigurationFault(world::ObservationFault::WrongThread);return;
  }
  capture_=CurrentWorldRecording();
  if(capture_&&capture_->collector)token_=capture_->collector->BeginDeliriumReconfiguration(world::PackWord(object));
}
WorldEntityReconfigurationScope::~WorldEntityReconfigurationScope() noexcept {
  const bool normal=returned_&&std::uncaught_exceptions()==exceptions_;
  if(!normal)world::ReportEntityReconfigurationFault(world::ObservationFault::NativeException);
  if(token_&&capture_&&capture_->collector)capture_->collector->EndDeliriumReconfiguration(token_,normal);
}
void WorldEntityReconfigurationScope::Returned() noexcept {
  returned_=true;if(counters_.completed)counters_.completed->fetch_add(1,std::memory_order_relaxed);
}
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
  auto *collector = Collector();
  return collector
             ? (native ? collector->NativeEnter(kind, object, args, count)
                       : collector->ObserveEnter(kind, object, args, count))
             : 0;
}
void Leave(std::uint32_t call, std::uint32_t result, bool normal,
           bool native) noexcept {
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
void InitializeWorldBridge() noexcept { world::SetObservationSink(&sink); }
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
    recording->observation.RequestStop();
    collector->Stop();
  }
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
    return;
  }
  collector->BeginOuter(loop, OwnerToken(Owner::Manager), RunActive());
  // 原生run/room撤销即取消活动上下文，不等待可选Lua回调或下一次30tick采集。
  auto &location=recording->hierarchy;
  if(location.valid&&(location.run!=collector->CurrentRunToken()||location.room!=collector->CurrentRoomToken())) {
    location.Invalidate(location.run!=collector->CurrentRunToken());
    recording->latest_room={}; recording->latest_level={}; recording->latest_continuity={};
  }
  WorldRefreshAnchors();
}
world::PlayerAnchor WorldPlayerAnchor(unsigned ordinal, void *player) noexcept {
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
  if (!topology.known || !topology.unique || !topology.Alive() ||
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
void WorldRefreshAnchors() noexcept {
  auto *collector = Collector();
  auto *r = Runtime();
  if (!collector || !r)
    return;
  std::array<world::PlayerAnchor, world::kControlAnchorCapacity> values{};
  for (unsigned ordinal = 0; ordinal < r->session.ParticipantLimit();
       ++ordinal) {
    if (!r->session.HasParticipant(ordinal) || r->finished || r->retiring ||
        r->failure || r->blocks || !r->players[ordinal].BoundValid() ||
        InBlockedContext())
      continue;
    const auto group = ReadTopology(
        ordinal, ordinal ? r->Remote(ordinal).device.Object() : nullptr);
    if (!group.known || !group.unique || !group.Alive() ||
        group.pair != r->players[ordinal].Bound() ||
        !SameControlGroup(group, r->topology[ordinal]))
      continue;
    for (unsigned b = 0; b < group.body_count; ++b) {
      if (!group.bodies[b].CanConsume())
        continue;
      values[ordinal * kMaximumGroupBodies + b] = {
          world::PackWord(group.bodies[b].player),
          world::PackWord(group.pair.device),
          group.pair.key,
          SessionGeneration(*r),
          r->players[ordinal].Generation(),
          true};
    }
  }
  std::array<world::PlayerAnchor,world::kControlAnchorCapacity> previous{};
  for(unsigned slot=0;slot<values.size();++slot) previous[slot]=collector->CurrentAnchor(slot);
  // 根切换会重排身体槽；先撤销全部变化槽，再赋新值，避免同一个Entity身份被旧槽清掉。
  for (unsigned slot = 0; slot < values.size(); ++slot)
    if (!(collector->CurrentAnchor(slot) == values[slot]))
      collector->RevokePlayer(slot);
  for (unsigned slot = 0; slot < values.size(); ++slot)
    collector->AnchorPlayer(slot, values[slot]);
  for(unsigned slot=0;slot<values.size();++slot) if(!(previous[slot]==values[slot]))
    ObservationControlChange(slot,previous[slot],values[slot],"observed_topology_or_lifecycle_refresh");
}
// 纯值来源只在已复核的普通窗口使用缓存拓扑；不额外全图扫描，不扩大输入消费资格。
void ObservationPlayerMembership(void *object,ObservationSubjectValue &value) noexcept {
  auto c=CurrentWorldRecording();auto *r=Runtime();
  if(!IsMain()||!c||!c->location_window||!r||r->finished||r->retiring||r->failure||r->blocks) return;
  for(unsigned ordinal=0;ordinal<r->session.ParticipantLimit();++ordinal) {
    if(!r->session.HasParticipant(ordinal)||!r->players[ordinal].BoundValid()) continue;
    const auto &topology=r->topology[ordinal];
    if(!topology.known||!topology.unique||topology.pair!=r->players[ordinal].Bound()) continue;
    for(unsigned i=0;i<topology.body_count;++i) if(topology.bodies[i].player==object) {
      if(value.ordinal<4&&value.ordinal!=ordinal) return;
      const auto session=SessionGeneration(*r);
      if(c->control_sessions[ordinal]!=session) {
        c->control_sessions[ordinal]=session;
        c->participant_keys[ordinal]=c->observation.AllocateRelatedSubject(world::observation::IdentitySpace::Participant);
        c->control_group_keys[ordinal]=c->observation.AllocateRelatedSubject(world::observation::IdentitySpace::ControlGroup);
      }
      value.ordinal=ordinal;value.participant=c->participant_keys[ordinal].local;value.group=c->control_group_keys[ordinal].local;
      value.control_session=session;value.binding_revision=r->players[ordinal].Generation();value.device_revision=c->device_relation_revisions[ordinal];
      value.group_known=true;value.body_role=unsigned(topology.bodies[i].role);value.body_active=topology.bodies[i].active;value.can_consume=topology.bodies[i].CanConsume();return;
    }
  }
}
void WorldRevokePlayer(unsigned ordinal) noexcept {
  if (ordinal >= core::kMaximumParticipants)
    return;
  if (auto *collector = Collector())
    for (unsigned b = 0; b < kMaximumGroupBodies; ++b) {
      const auto slot=ordinal*kMaximumGroupBodies+b;
      const auto before=collector->CurrentAnchor(slot);
      collector->RevokePlayer(slot);
      if(before.valid) ObservationControlChange(slot,before,{},"explicit_binding_revocation");
    }
}
void WorldPlayerReturned(unsigned ordinal, const world::PlayerAnchor &before,
                         std::uint64_t tick, bool normal, bool body) noexcept {
  auto *collector = Collector();
  auto *r = Runtime();
  if (!collector || !r || ordinal >= core::kMaximumParticipants)
    return;
  void *player = reinterpret_cast<void *>(uintptr_t(before.player));
  const auto after = WorldPlayerAnchor(ordinal, player);
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
  if (auto *collector = Collector()) {
    WorldRefreshAnchors();
    collector->NativeReturned({boundary.loop, OwnerToken(Owner::Manager), tick,
                               boundary.scope_token,
                               boundary.kind == core::TickKind::StandardRoom
                                   ? world::AuthorityKind::StandardRoom
                                   : world::AuthorityKind::OddPlayer});
  }
}
void WorldPreparePostState(bool normal) noexcept {
  if (auto *collector = Collector()) {
    if (!collector->Participating()) {
      prepared_post = {};
      frame_prepared = true;
      return;
    }
    WorldRefreshAnchors();
    ObserveFailures(*collector);
    prepared_post = collector->PreparePostState(normal);
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
  if (auto *collector = Collector())
    collector->PublishPostState(committed, post);
}
void WorldFrameEnd() noexcept {
  if (auto *collector = Collector())
    collector->EndOuter();
}

WorldBoundaryScope::WorldBoundaryScope(WorldBoundary kind, const void *object,
                                       bool enabled) noexcept
    : kind_(kind), exceptions_(std::uncaught_exceptions()) {
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
