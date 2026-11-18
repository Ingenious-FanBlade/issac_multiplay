#include "game/authority/lifecycle.h"
#include "binding/offsets.h"
#include "game/common/context.h"
#include "game/participants/device_k.h"
#include "game/input/input_a.h"
#include "game/authority/persistence.h"
#include "game/participants/remote_life.h"
#include "game/authority/state.h"
#include "game/authority/tick.h"
#include "game/observation/world_bridge.h"
#include "runtime.h"
#include "world/common/frame_read_regions.h"
#include <algorithm>

namespace cs::game {
namespace {
// 观测统计汇总所有Peer；不让旧的单B计数冒充全体admission。
uint64_t AdmittedTotal(const SessionRuntime &r) noexcept {
  uint64_t total = 0;
  for (const auto &remote : r.remotes)
    total += remote.peer.admitted;
  return total;
}
// 只从已读取的 game 字段判断加载窗口，不把内部 Room/Player 更新当成加载。
bool LoadingFromFlags(const Topology &value) noexcept {
  return value.known && value.run &&
         (value.starting || value.room_transition || value.stage_transition);
}
unsigned LoadingReasons(const Topology &value) noexcept {
  unsigned reasons = 0;
  if (value.starting)
    reasons |= 1;
  if (value.room_transition)
    reasons |= 2;
  if (value.stage_transition)
    reasons |= 4;
  return reasons;
}
// 加载 owner 事件位到 cs::Owner 的映射；只用于记录 owner token。
Owner OwnerForEvent(unsigned bit) noexcept {
  if (bit == StartOwner)
    return Owner::Start;
  if (bit == RestoreOwner)
    return Owner::Restore;
  return Owner::Lifecycle;
}
// 进入加载窗口的具体来源：区分 starting、换房与换层，便于 V13 归约。
const char *LoadingEnterReason(const Topology &value) noexcept {
  if (value.starting)
    return "starting_enter";
  if (value.room_transition)
    return "room_transition_enter";
  if (value.stage_transition)
    return "stage_transition_enter";
  return "loading_enter";
}
} // namespace
// 两个 ordinal 对称失效；旧地址只用于识别中性查询，绝不据此解引用旧 Player。
void InvalidateBinding(SessionRuntime &r, const char *reason,
                       bool defer) noexcept {
  // 先推进观测 epoch：失效前后的查询绝不进入同一比较段，reason 与 epoch
  // 一起记录，便于把每条失效归因到具体资格变化。
  ++r.query_epoch;
  if (r.loading.active)
    ++r.loading.invalidations;
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i) {
    WorldRevokePlayer(i);
    if (r.players[i].BoundValid()) {
      r.controlled_player[i] = r.players[i].Bound().player;
      r.players[i].Invalidate();
      Trace(
          "binding",
          "ordinal=%u bound=0 generation=%llu reason=%s epoch=%llu loop=%llu "
          "reason_class=%s session_generation=%llu participant_generation=%llu",
          i, r.players[i].Generation(), reason, r.query_epoch, r.loop,
          EligibilityReasonClass(reason), SessionGeneration(r),
          r.players[i].Generation());
    }
  }
  for (auto &remote : r.remotes)
    remote.life.pending_reason = reason;
  if (defer)
    r.defer_through = (std::max)(r.defer_through, r.loop);
  DiscardInputEdges();
  DiscardPeerWindow(r);
}
void BeginLoadingEpisode(SessionRuntime &r, unsigned reasons,
                         const char *source, bool first_menu) noexcept {
  auto &episode = r.loading;
  if (episode.active) {
    episode.reasons |= reasons;
    return;
  }
  ++r.loading_episodes;
  episode = {};
  episode.id = r.loading_episodes;
  episode.active = true;
  episode.reason = source ? source : "loading_enter";
  episode.first_menu = first_menu;
  episode.start_loop = r.loop;
  episode.start_completed = r.clock.Completed();
  episode.overrides_at_start = r.overrides;
  episode.admitted_at_start = AdmittedTotal(r);
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i)
    episode.queue_at_start[i] = r.players[i].Depth();
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i)
    episode.generation_at_start[i] = r.players[i].Generation();
  episode.reasons = reasons;
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i) {
    const auto progress = r.players[i].Progress(r.clock.Completed());
    episode.submit_at_start[i] = progress.submit_count;
    episode.resolved_tick_at_start[i] = progress.last_resolved_tick;
  }
  Trace("loading",
        "event=begin episode=%llu source=%s loop=%llu tick=%llu reasons=%u "
        "first_menu=%u queue=%llu,%llu generation=%llu,%llu overrides=%llu "
        "admitted=%llu session_generation=%llu a_generation=%llu "
        "b_generation=%llu "
        "submit=%llu,%llu resolved_tick=%llu,%llu",
        episode.id, episode.reason, r.loop, episode.start_completed,
        episode.reasons, episode.first_menu, episode.queue_at_start[0],
        episode.queue_at_start[1], episode.generation_at_start[0],
        episode.generation_at_start[1], episode.overrides_at_start,
        episode.admitted_at_start, SessionGeneration(r),
        r.players[0].Generation(), r.players[1].Generation(),
        episode.submit_at_start[0], episode.submit_at_start[1],
        episode.resolved_tick_at_start[0], episode.resolved_tick_at_start[1]);
}
void EndLoadingEpisode(SessionRuntime &r, const char *reason) noexcept {
  auto &episode = r.loading;
  if (!episode.active)
    return;
  episode.active = false;
  episode.end_reason = reason ? reason : "closed";
  episode.end_loop = r.loop;
  episode.end_completed = r.clock.Completed();
  episode.overrides_at_end = r.overrides;
  episode.admitted_at_end = AdmittedTotal(r);
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i)
    episode.queue_at_end[i] = r.players[i].Depth();
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i)
    episode.generation_at_end[i] = r.players[i].Generation();
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i) {
    const auto progress = r.players[i].Progress(r.clock.Completed());
    episode.submit_at_end[i] = progress.submit_count;
    episode.resolved_tick_at_end[i] = progress.last_resolved_tick;
  }
  Trace("loading",
        "event=end episode=%llu source=%s loop=%llu tick=%llu reason=%s "
        "reasons=%u owners=%u invalidations=%llu queries=%llu nonzero=%llu "
        "unknown=%llu ticks=%llu overrides=%llu..%llu admitted=%llu..%llu "
        "queue=%llu,%llu->%llu,%llu generation=%llu,%llu->%llu,%llu "
        "session_generation=%llu submit_start=%llu,%llu submit_end=%llu,%llu "
        "resolved_tick_start=%llu,%llu resolved_tick_end=%llu,%llu",
        episode.id, episode.reason, episode.end_loop, episode.end_completed,
        reason, episode.reasons, episode.owners, episode.invalidations,
        episode.queries, episode.queries_nonzero, episode.queries_unknown,
        episode.end_completed > episode.start_completed
            ? episode.end_completed - episode.start_completed
            : 0,
        episode.overrides_at_start, episode.overrides_at_end,
        episode.admitted_at_start, episode.admitted_at_end,
        episode.queue_at_start[0], episode.queue_at_start[1],
        episode.queue_at_end[0], episode.queue_at_end[1],
        episode.generation_at_start[0], episode.generation_at_start[1],
        episode.generation_at_end[0], episode.generation_at_end[1],
        SessionGeneration(r), episode.submit_at_start[0],
        episode.submit_at_start[1], episode.submit_at_end[0],
        episode.submit_at_end[1], episode.resolved_tick_at_start[0],
        episode.resolved_tick_at_start[1], episode.resolved_tick_at_end[0],
        episode.resolved_tick_at_end[1]);
}
void RecordLoadingQuery(bool nonzero, bool known_context) noexcept {
  if (!Settings.diagnostics)
    return;
  auto *r = Runtime();
  if (!r || !r->loading.active)
    return;
  ++r->loading.queries;
  if (nonzero)
    ++r->loading.queries_nonzero;
  if (!known_context)
    ++r->loading.queries_unknown;
}
// Start 进入时固定“首次菜单加载”资格，并在 Start/Lifecycle owner 正常返回后
// 把稳定入口延后到后续 loop；加载内部回调不提前授予绑定/创建 K 的资格。
void NoteLoadingOwnerEntered(unsigned bit) noexcept {
  if (!IsMain())
    return;
  auto *r = Runtime();
  if (!r || r->finished)
    return;
  // 初次菜单加载只允许尚无设备/Join/绑定的全体名单；不会只检查首个远端。
  bool pristine = !r->run_observed;
  for (const auto &remote : r->remotes)
    pristine &=
        !remote.join.complete && remote.device.State() == DeviceKState::Empty;
  for (const auto &player : r->players)
    pristine &= !player.BoundValid();
  const bool qualified =
      bit == StartOwner && r->mode == core::SessionMode::Host &&
      r->session.Status().state == core::SessionState::Warmup && pristine &&
      !RunActive() && !r->retiring && !r->failure;
  if (bit == StartOwner)
    r->start_qualified = qualified;
  if (r->loading.active)
    r->loading.owners |= bit;
  const auto owner = OwnerForEvent(bit);
  Trace("loading_owner",
        "event=enter bit=%u owner=%u owner_token=%llu qualification=%u "
        "loop=%llu tick=%llu run=%u run_observed=%u joined=%u device=%s "
        "bound=%u,%u starting=%u stage=%d session_generation=%llu episode=%llu",
        bit, unsigned(owner), OwnerToken(owner), qualified, r->loop,
        r->clock.Completed(), unsigned(RunActive()), r->run_observed,
        r->join.complete, DeviceKStateName(r->device.State()),
        r->players[0].BoundValid(), r->players[1].BoundValid(),
        unsigned(r->topology[0].starting), r->topology[0].stage_transition,
        SessionGeneration(*r), r->loading.active ? r->loading.id : 0);
  if (qualified)
    OpenPersistenceWindow(PersistenceWindowKind::Continue, "first_menu_start",
                          0);
}
void NoteLoadingOwnerReturned(unsigned bit, bool normal) noexcept {
  if (!IsMain())
    return;
  auto *r = Runtime();
  if (!r || r->finished)
    return;
  if (r->loading.active)
    r->loading.owners |= bit;
  const auto owner = OwnerForEvent(bit);
  Trace("loading_owner",
        "event=return bit=%u owner=%u owner_token=%llu normal=%u loop=%llu "
        "tick=%llu starting=%u stage=%d room_transition=%d generation=%llu "
        "session_generation=%llu episode=%llu a_generation=%llu "
        "b_generation=%llu",
        bit, unsigned(owner), OwnerToken(owner), unsigned(normal), r->loop,
        r->clock.Completed(), unsigned(r->topology[0].starting),
        r->topology[0].stage_transition, r->topology[0].room_transition,
        SessionGeneration(*r), SessionGeneration(*r),
        r->loading.active ? r->loading.id : 0, r->players[0].Generation(),
        r->players[1].Generation());
  if (!normal) {
    FailRuntime("loading_owner_exception");
    return;
  }
  // owner 正常返回后仍不能在同一 loop 作为稳定 Authority 入口。
  r->defer_through = (std::max)(r->defer_through, r->loop);
  if (bit == StartOwner)
    r->start_qualified = false;
}
// 已有 ST4 指令证实原函数先检查 +0x4AD10；只读该位，不写原生启动状态。
bool StartGameRequested(void *manager) noexcept {
  if (!IsMain())
    return false;
  bool requested = false;
  if (!ReadAt(manager, offset::Starting, requested)) {
    ContextFault("starting_flag_unreadable");
    return false;
  }
  return requested;
}
LoadingOwnerScope::LoadingOwnerScope(bool start_owner, bool enabled) noexcept
    : bit_(start_owner ? StartOwner : LifecycleOwner),
      main_(IsMain() && enabled), exceptions_(std::uncaught_exceptions()) {
  if (main_)
    NoteLoadingOwnerEntered(bit_);
}
LoadingOwnerScope::~LoadingOwnerScope() noexcept {
  if (!main_)
    return;
  NoteLoadingOwnerReturned(bit_, exceptions_ == std::uncaught_exceptions());
}
namespace {
// 完整图已在当前边界确认后，consumer只检查当前身体、设备及全局阻断字段。
// 任一变化都退回完整图读取；不在原生调用之间缓存未知对象，也不因缓存授予绑定。
// 只复用页权限。所有身体和设备身份字段仍在当前 consumer 内实读。
class ConsumerReadWindow final {
public:
  bool Copy(const void *source,void *output,std::size_t size) noexcept {
    return world::CheckedNativeCopy(reinterpret_cast<std::uintptr_t>(source),output,size,world::FrameRegionReader::Consumer);
  }
  template<class T>bool At(const void *object,std::size_t offset,T &value) noexcept {
    const auto address=reinterpret_cast<std::uintptr_t>(object);
    return object&&offset<=UINTPTR_MAX-address&&Copy(reinterpret_cast<const void *>(address+offset),&value,sizeof(value));
  }
};
bool StableConsumerSnapshot(const SessionRuntime &r, void *player) noexcept {
  if (!IsMain() || !player || !r.pipeline.SimulationTick() || r.blocks || !r.observed ||
      r.lifecycle_loop != r.loop || r.lifecycle_epoch != r.query_epoch)
    return false;
  ConsumerReadWindow reads;
  const auto &host = r.topology[0];
  void *game = GamePointer(), *room = nullptr, *manager = nullptr;
  bool run = false, starting = false, focused = false, pause_on_focus = false;
  int pause = 0, room_transition = 0, stage_transition = 0;
  unsigned stage = 0, stage_type = 0, index = 0, dimension = 0;
  if (!host.known || game != host.room.game ||
      !reads.At(game, offset::RunActive, run) || run != host.run ||
      !reads.At(game, offset::CurrentRoom, room) || room != host.room.room ||
      !reads.At(game, 0, stage) || stage != host.room.stage ||
      !reads.At(game, 4, stage_type) || stage_type != host.room.stage_type ||
      !reads.At(game, offset::RoomIndex, index) || index != host.room.index ||
      !reads.At(game, offset::Dimension, dimension) ||
      dimension != host.room.dimension ||
      !reads.At(game, offset::PauseScreen, pause) || pause != host.pause ||
      !reads.At(game, offset::RoomTransition, room_transition) ||
      room_transition != host.room_transition ||
      !reads.At(game, offset::StageTransition, stage_transition) ||
      stage_transition != host.stage_transition ||
      !reads.Copy(reinterpret_cast<void *>(Base + offset::Manager), &manager,
                  sizeof(manager)) ||
      !reads.At(manager, offset::Starting, starting) ||
      starting != host.starting || !reads.At(manager, offset::Focus, focused) ||
      focused != host.focused ||
      !reads.At(manager, offset::PauseOnFocusLost, pause_on_focus) ||
      pause_on_focus != host.pause_on_focus_lost)
    return false;
  for (unsigned ordinal = 0; ordinal < r.session.ParticipantLimit();
       ++ordinal) {
    if (!r.session.HasParticipant(ordinal))
      continue;
    const auto &group = r.topology[ordinal];
    for (unsigned b = 0; b < group.body_count; ++b) {
      const auto &body = group.bodies[b];
      if (body.player != player)
        continue;
      unsigned type = 0, variant = 0;
      int character = -1, controller = -1, device_id = -1;
      bool valid = false, exists = false, dead = false, removed = false,
           ghost = false, enabled = false;
      if (!reads.At(player, offset::EntityType, type) || type != 1 ||
          !reads.At(player, offset::EntityVariant, variant) || variant != 0 ||
          !reads.At(player, offset::PlayerType, character) ||
          character != body.character ||
          !reads.At(player, offset::ControllerIndex, controller) ||
          controller != body.controller ||
          !reads.At(player, offset::EntityValid, valid) || valid != body.valid ||
          !reads.At(player, offset::EntityExists, exists) ||
          exists != body.exists || !reads.At(player, offset::EntityDead, dead) ||
          dead != body.dead ||
          !reads.At(player, offset::EntityRemoved, removed) ||
          removed != body.removed ||
          !reads.At(player, offset::CoopGhost, ghost) || ghost != body.ghost ||
          !reads.At(group.pair.device, offset::DeviceId, device_id) ||
          device_id != group.pair.key ||
          !reads.At(group.pair.device, offset::DeviceEnabled, enabled) ||
          !enabled)
        return false;
      constexpr size_t offsets[] = {0x1D84, 0x1E4C, 0x1E50, 0x267C};
      for (unsigned relation = 0; relation < 4; ++relation) {
        void *target = nullptr;
        if (!reads.At(player, offsets[relation], target) ||
            target != body.relations[relation])
          return false;
      }
      return true;
    }
  }
  return false;
}
} // namespace
void RefreshLifecycle(SessionRuntime &r, bool authority,
                      void *consumer_player) noexcept {
  if (r.finished || r.retiring || r.join_ordinal || r.control_lifecycle_depth ||
      !r.core_started)
    return;
  if (!authority && !Settings.repeat_topology_reads &&
      StableConsumerSnapshot(r, consumer_player)) {
    ++r.lifecycle_reuses;
    return;
  }
  ++r.lifecycle_refreshes;
  const bool host_mode = r.mode == core::SessionMode::Host;
  TopologyGroups values{};
  TopologyDevices devices{};
  unsigned mask = 1;
  if (host_mode)
    for (unsigned i = 1; i < core::kMaximumParticipants; ++i)
      if (r.session.HasParticipant(i) && r.Remote(i).join.complete && !r.Remote(i).retired) {
        mask |= 1u << i;
        devices[i] = r.Remote(i).device.Object();
      }
  const auto conflicts = ReadTopologies(mask, devices, values);
  if (Runtime() != &r)
    return;
  // 不同参与者不得共享身体（包括备份）。冲突只撤销相关组，不借查询回退串用物理设备。
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i)
    if (r.session.HasParticipant(i) && (conflicts & (1u << i))) {
      if (r.players[i].BoundValid())
        r.players[i].Invalidate();
      WorldRevokePlayer(i);
    }
  const auto &value = values[0];
  if (value.known && !value.run) {
    // 主菜单握手后的 Warmup 无限等待首次 run；真正经历 run 后离开才是
    // teardown。
    if (host_mode && !r.run_observed) {
      r.topology = values;
      if (r.loading.active)
        EndLoadingEpisode(r, "run_left");
      r.blocks = Lifecycle;
      r.observed = false;
      DiscardInputEdges();
      DiscardPeerWindow(r);
      return;
    }
    r.run_torn_down = true;
    StopSession(core::SessionCause::RunTeardown);
    return;
  }
  // 1. 先识别加载窗口：starting/room/stage 及其 Start/Restore/Lifecycle owner。
  const bool run = value.known && value.run;
  const bool loading = LoadingFromFlags(value);
  // 2. run_observed 只表示“观察过非加载的有效 run”，首次 Continue 不被当成
  // 已有 run。
  if (run && !loading)
    r.run_observed = true;
  const auto old_blocks = r.blocks;
  unsigned blocks = 0;
  if (!value.known || !value.room.room || !ContextHealthy())
    blocks |= Lifecycle;
  if (r.console_blocked ||
      (run &&
       (value.pause > 0 || (!value.focused && value.pause_on_focus_lost))))
    blocks |= Pause;
  if (loading)
    blocks |= Loading;
  if (loading) {
    if (!r.loading.active)
      BeginLoadingEpisode(
          r, LoadingReasons(value), LoadingEnterReason(value),
          PersistenceWindowActive(PersistenceWindowKind::Continue));
  } else if (r.loading.active) {
    EndLoadingEpisode(r, "loading_clear");
  }
  // 3. 加载中只意味着不可绑定；人数/身份/归属矛盾只在非加载的稳定观察判断。
  if (host_mode && run && !loading) {
    for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
      auto &remote = r.Remote(i);
      if (!remote.join.complete || remote.retiring || remote.retired)
        continue;
      const auto action = ObserveRemoteLife(r, value, values[i], i);
      if (action == RemoteLifeAction::PeerLost) {
        RequestRemoteRetirement(r, i, core::SessionCause::PeerLost);
      } else if (action == RemoteLifeAction::Invalidate) {
        InvalidateRemoteBinding(r, "remote_lifecycle_unavailable", i);
      }
    }
  }
  // 4. A 的房间/生命/精确关联变化；B 的地址与归属变化由 ObserveRemoteLife
  // 处理，避免死亡状态被反复迭代成新的失效。
  if (r.observed && run && !loading) {
    if (!(values[0].room == r.topology[0].room))
      InvalidateBinding(r, "room_identity_changed", true);
    else if (!SameLife(values[0], r.topology[0]) ||
             !SameControlGroup(values[0], r.topology[0]) ||
             values[0].unique != r.topology[0].unique) {
      // Host死亡/复活或角色切换只撤销自己的输入；其他Peer和Authority继续，不形成全员等待。
      WorldRevokePlayer(0);
      if (r.players[0].BoundValid())
        r.players[0].Invalidate();
      ++r.query_epoch;
      DiscardInputEdges();
      Trace("binding",
            "ordinal=0 bound=0 reason=host_control_group_changed "
            "generation=%llu loop=%llu",
            r.players[0].Generation(), r.loop);
    }
  }
  if (r.loop <= r.defer_through)
    blocks |= Lifecycle;
  const unsigned entered = blocks & ~old_blocks, left = old_blocks & ~blocks;
  if (entered) {
    // 进入阻断时记录具体来源：加载按 starting/换房/换层细分，便于 V13 归约。
    const char *reason = (entered & Loading) ? LoadingEnterReason(value)
                         : (entered & Pause) ? "pause_enter"
                                             : "lifecycle_enter";
    InvalidateBinding(r, reason, false);
  }
  if (left) {
    const char *reason = (left & Loading) ? "loading_leave"
                         : (left & Pause) ? "pause_leave"
                                          : "lifecycle_leave";
    InvalidateBinding(r, reason, false);
  }
  r.blocks = blocks;
  r.topology = values;
  r.observed = true;
  if (old_blocks != blocks)
    Trace("block_interval",
          "from=%u to=%u loop=%llu console=%u pause=%d focus=%u "
          "room_transition=%d stage_transition=%d reason=%s",
          old_blocks, blocks, r.loop, r.console_blocked, value.pause,
          value.focused, value.room_transition, value.stage_transition,
          value.reason);
  // 5. 只有稳定且未阻断的 Authority 入口才重新唯一绑定；B 的死亡状态在此
  // 之前已拒绝输入，不会因为 blocks 清零而被绕过。
  bool topology_just_read = true;
  if (authority && !blocks) {
    if (r.loop > r.binding_defer_through[0] && !r.control_quarantined[0] &&
        !r.players[0].BoundValid() && values[0].unique && values[0].Alive()) {
      if (!r.players[0].Bind(values[0].pair)) {
        FailRuntime("unique_binding_rejected");
        return;
      }
      r.controlled_player[0] = values[0].pair.player;
      ++r.query_epoch;
      Trace("binding",
            "ordinal=0 bound=1 generation=%llu loop=%llu "
            "reason=stable_authority session_generation=%llu "
            "participant_generation=%llu epoch=%llu",
            r.players[0].Generation(), r.loop, SessionGeneration(r),
            r.players[0].Generation(), r.query_epoch);
    }
    if (host_mode)
      for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
        auto &remote = r.Remote(i);
        if (!remote.join.complete || remote.retiring || remote.retired ||
            r.players[i].BoundValid())
          continue;
        const bool revived = remote.life.status == RemoteLifeStatus::Revived;
        if (remote.life.status == RemoteLifeStatus::Alive || revived)
          if (BindRevivedRemote(r, revived, i))
            topology_just_read = false;
      }
  }
  WorldRefreshAnchors(topology_just_read);
  ObservePersistenceWindows(r);
  r.lifecycle_loop = r.loop;
  r.lifecycle_epoch = r.query_epoch;
}
// Warmup 的稳定 Authority 只负责 K 的一次创建/等待 Join，不分配输入或推进
// tick。Active 之后只要求 A 与会话可用，允许 B 为 Lost/未绑定。
bool ReadyForAuthority(SessionRuntime &r) noexcept {
  if (r.control_lifecycle_depth)
    return false;
  RefreshLifecycle(r, true);
  if (r.retiring || r.blocks ||
      (!r.players[0].BoundValid() &&
       r.session.Status().state != core::SessionState::Active))
    return false;
  if (r.mode == core::SessionMode::Host) {
    for (unsigned i = 1; i < core::kMaximumParticipants; ++i) {
      auto &remote = r.Remote(i);
      if (!remote.peer.established || remote.retiring || remote.retired)
        continue;
      if (remote.device.State() == DeviceKState::Empty) {
        if (!remote.device.CreateOnce())
          RequestRemoteRetirement(r, i, core::SessionCause::PeerProtocol);
      } else if (remote.device.State() != DeviceKState::Registered) {
        RequestRemoteRetirement(r, i, core::SessionCause::PeerLost);
      }
    }
  }
  if (r.session.Status().state == core::SessionState::Warmup) {
    if (!r.session.Activate(r.players[0].BoundValid(),
                            r.players[1].BoundValid())) {
      FailRuntime("activate_failed");
      return false;
    }
    DiscardPeerWindow(r);
    Trace("session",
          "from=Warmup to=Active mode=%s reason=BindingsReady generation=%llu "
          "loop=%llu",
          core::SessionModeName(r.mode), r.session.Identity().generation,
          r.loop);
  }
  return r.session.AcceptsInput();
}
void ObserveConsole(bool blocked) noexcept {
  if (!IsMain())
    return;
  auto *r = Runtime();
  if (!r || r->finished || r->retiring ||
      (!HasOwner(Owner::Game) && !HasOwner(Owner::Odd)))
    return;
  if (r->console_blocked != blocked)
    Trace("paused", "active=%u loop=%llu source=console_original", blocked,
          r->loop);
  r->console_blocked = blocked;
  RefreshLifecycle(*r, false);
}
void NewRoomReturned() noexcept {
  if (!IsMain())
    return;
  if (auto *r = Runtime(); r && !r->finished && !r->retiring) {
    InvalidateBinding(*r, "new_room_returned", true);
    RefreshLifecycle(*r, false);
  }
}
// 在原生清 Player 之前关闭会话；之后退休不再对旧 B 调 RemoveCoPlayer。
void RunTeardownBeginning(bool native_exit) noexcept {
  if (!IsMain())
    return;
  if (auto *r = Runtime(); r && !r->finished) {
    r->run_torn_down = true;
  }
  // 窗口属于真实原生退出，不依赖 Host 仍活动；只观察，不重建会话或旧绑定。
  if (native_exit)
    OpenPersistenceWindow(PersistenceWindowKind::RunExit, "native_exit");
  StopSession(core::SessionCause::RunTeardown);
}
std::string LoadingFields(const SessionRuntime *r) {
  std::string out;
  const auto n = [&](const char *key, uint64_t value) {
    out += key;
    out += '=';
    out += std::to_string(value);
    out += '\n';
  };
  const LoadingEpisode *episode = r ? &r->loading : nullptr;
  n("loading_episodes", r ? r->loading_episodes : 0);
  n("loading_active", episode && episode->active);
  n("loading_episode", episode ? episode->id : 0);
  n("loading_reasons", episode ? episode->reasons : 0);
  n("loading_owners", episode ? episode->owners : 0);
  n("loading_invalidations", episode ? episode->invalidations : 0);
  n("loading_queries", episode ? episode->queries : 0);
  n("loading_queries_nonzero", episode ? episode->queries_nonzero : 0);
  n("loading_queries_unknown", episode ? episode->queries_unknown : 0);
  n("loading_first_menu", episode && episode->first_menu);
  n("loading_admitted_start", episode ? episode->admitted_at_start : 0);
  n("loading_admitted_end", episode ? episode->admitted_at_end : 0);
  n("loading_end_known", episode && episode->id && !episode->active);
  n("loading_start_loop", episode ? episode->start_loop : 0);
  n("loading_end_loop", episode ? episode->end_loop : 0);
  n("loading_completed_start", episode ? episode->start_completed : 0);
  n("loading_completed_end", episode ? episode->end_completed : 0);
  n("loading_admitted_current", r ? AdmittedTotal(*r) : 0);
  for (unsigned i = 0; i < core::kMaximumParticipants; ++i) {
    const std::string prefix =
        std::string("loading.participant.") + std::to_string(i) + ".";
    n((prefix + "queue_start").c_str(),
      episode ? episode->queue_at_start[i] : 0);
    n((prefix + "queue_end").c_str(), episode ? episode->queue_at_end[i] : 0);
    n((prefix + "generation_start").c_str(),
      episode ? episode->generation_at_start[i] : 0);
    n((prefix + "generation_end").c_str(),
      episode ? episode->generation_at_end[i] : 0);
    n((prefix + "submit_start").c_str(),
      episode ? episode->submit_at_start[i] : 0);
    n((prefix + "submit_end").c_str(), episode ? episode->submit_at_end[i] : 0);
    n((prefix + "resolved_tick_start").c_str(),
      episode ? episode->resolved_tick_at_start[i] : 0);
    n((prefix + "resolved_tick_end").c_str(),
      episode ? episode->resolved_tick_at_end[i] : 0);
  }
  out += "loading_source=" + std::string(episode ? episode->reason : "none") +
         "\n";
  out += "loading_end_reason=" +
         std::string(episode ? episode->end_reason : "none") + "\n";
  n("start_qualified", r && r->start_qualified);
  return out;
}
} // namespace cs::game
