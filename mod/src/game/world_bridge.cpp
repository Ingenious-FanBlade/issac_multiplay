#include "game/world_bridge.h"
#include "binding/world_layout.h"
#include "game/bridge.h"
#include "game/context.h"
#include "game/state.h"
#include "game/tick.h"
#include "world/observe.h"
#include <exception>

namespace cs::game {
WorldRecording::~WorldRecording() {
  if (faults.unreadable) {
    VirtualFree(reinterpret_cast<void *>(uintptr_t(faults.unreadable)), 0, MEM_RELEASE);
    ledger.Release(65536);
  }
}
namespace {
std::shared_ptr<WorldRecording> recording;
std::uint64_t fault_sequence = 0;
core::TickPostState prepared_post{};
bool frame_prepared = false;
bool activation_pending = false;
world::Collector *Collector() noexcept {
  return IsMain() && recording ? recording->collector.get() : nullptr;
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
const world::ObservationSink sink{Enter, Leave};
world::Stage StageFor(WorldBoundary kind) noexcept {
  return kind == WorldBoundary::Owner  ? world::Stage::Owner
         : kind == WorldBoundary::Room ? world::Stage::Room
                                       : world::Stage::Tail;
}
} // namespace
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
      !Settings.world_record_enabled || !Capabilities[size_t(Group::World)] ||
      !Capabilities[size_t(Group::Loop)] ||
      !Capabilities[size_t(Group::Player)] || !next || !next->collector ||
      !next->storage.Ready() || !next->identities.Ready() ||
      !next->writer.Status().opened)
    return false;
  bool observations[world::kObservationCount]{};
  world::layout::ObservationMask(observations, world::kObservationCount);
  for (const auto registered : observations)
    if (!registered)
      return false;
  if (recording &&
      (!recording->collector->Drained() || !recording->storage.Closed() ||
       !recording->writer.Closed()))
    return false;
  recording = std::move(next);
  // 命令在 FrameBegin 内执行时，world 的前半个入口已过去；激活延后至下一个
  // Manager，不能以半个窗口补造完整阶段或把同帧出生归到 run=0。
  activation_pending = true;
  fault_sequence = world::ObservationFailureSequence();
  prepared_post = {};
  frame_prepared = false;
  return true;
}
void StopWorldRecording() noexcept {
  if (IsMain())
    activation_pending = false;
  if (auto *collector = Collector())
    collector->Stop();
}
bool DetachClosedWorld(const std::shared_ptr<WorldRecording> &closed) noexcept {
  if (!IsMain() || recording != closed || !closed || !closed->collector ||
      !closed->collector->Drained() || !closed->storage.Closed() ||
      !closed->writer.Closed())
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
    if (!collector->Activate())
      collector->Fault(world::Reason::PrepareFailed, "activate_prepared");
  }
  prepared_post = {};
  frame_prepared = false;
  const auto faults = world::ObservationFailureSequence();
  if (!collector->Participating()) {
    fault_sequence = faults;
    return;
  }
  if (faults != fault_sequence) {
    collector->Fault(world::Reason::WrongThread, "world_observation_fault");
    fault_sequence = faults;
  }
  collector->BeginOuter(loop, OwnerToken(Owner::Manager), RunActive());
  WorldRefreshAnchors();
}
world::PlayerAnchor WorldPlayerAnchor(unsigned ordinal) noexcept {
  world::PlayerAnchor anchor{};
  auto *collector = Collector();
  if (!collector || !collector->Recording())
    return anchor;
  auto *r = Runtime();
  if (!IsMain() || !r || r->finished || r->retiring || r->failure ||
      ordinal >= r->session.ParticipantCount() || r->blocks ||
      !r->players[ordinal].BoundValid() || InBlockedContext())
    return anchor;
  // ST7：B 的不可读/不唯一分支未必递增原绑定代次，因此这里独立复读拓扑。
  const auto topology =
      ReadTopology(ordinal, ordinal ? r->device.Object() : nullptr);
  if (!topology.known || !topology.unique || !topology.Alive() ||
      topology.pair != r->players[ordinal].Bound() ||
      !SameLife(topology, r->topology[ordinal]))
    return anchor;
  anchor.player = world::PackWord(topology.pair.player);
  anchor.device = world::PackWord(topology.pair.device);
  anchor.key = topology.pair.key;
  anchor.session = SessionGeneration(*r);
  anchor.generation = r->players[ordinal].Generation();
  anchor.valid = true;
  return anchor;
}
void WorldRefreshAnchors() noexcept {
  if (auto *collector = Collector())
    for (unsigned ordinal = 0; ordinal < 2; ++ordinal)
      collector->AnchorPlayer(ordinal, WorldPlayerAnchor(ordinal));
}
void WorldRevokePlayer(unsigned ordinal) noexcept {
  if (auto *collector = Collector())
    collector->RevokePlayer(ordinal);
}
void WorldPlayerReturned(unsigned ordinal, const world::PlayerAnchor &before,
                         std::uint64_t tick, bool normal, bool body) noexcept {
  if (auto *collector = Collector()) {
    auto *r = Runtime();
    const auto after = WorldPlayerAnchor(ordinal);
    collector->PlayerReturned(
        ordinal, before, after,
        r && r->pipeline.SimulationTick() == tick ? tick : 0, normal, body);
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
    const auto faults = world::ObservationFailureSequence();
    if (faults != fault_sequence) {
      collector->Fault(world::Reason::UnpairedScope,
                       "observation_unwind_or_thread");
      fault_sequence = faults;
    }
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
