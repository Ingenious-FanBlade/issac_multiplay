#include "game/lifecycle.h"
#include "game/context.h"
#include "game/device_k.h"
#include "game/input_a.h"
#include "game/persistence.h"
#include "game/remote_life.h"
#include "game/state.h"
#include "game/tick.h"
#include "runtime.h"
#include <algorithm>

namespace cs::game {
namespace {
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
// PeerLost 是防御路径：停止 admission/publication 后按既有次序退休，
// 不重试 Join，不再创建 K。
void RequestPeerLost(SessionRuntime &r, const char *reason) noexcept {
  Trace("session", "to=Retiring reason=PeerLost detail=%s loop=%llu",
        reason && reason[0] ? reason : "remote_conflict", r.loop);
  StopSession(core::SessionCause::PeerLost);
  if (!r.retiring)
    FailRuntime("peer_lost_retirement_rejected");
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
  for (unsigned i = 0; i < r.session.ParticipantCount(); ++i) {
    if (r.players[i].BoundValid()) {
      r.controlled_player[i] = r.players[i].Bound().player;
      r.players[i].Invalidate();
      Trace("binding",
            "ordinal=%u bound=0 generation=%llu reason=%s epoch=%llu loop=%llu "
            "reason_class=%s session_generation=%llu participant_generation=%llu",
            i, r.players[i].Generation(), reason, r.query_epoch, r.loop,
            EligibilityReasonClass(reason), SessionGeneration(r), r.players[i].Generation());
    }
  }
  r.life.pending_reason = reason;
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
  episode.admitted_at_start = r.peer.admitted;
  episode.queue_at_start = {r.players[0].Depth(), r.players[1].Depth()};
  episode.generation_at_start = {r.players[0].Generation(),
                                 r.players[1].Generation()};
  episode.reasons = reasons;
  for (unsigned i = 0; i < r.session.ParticipantCount(); ++i) {
    const auto progress = r.players[i].Progress(r.clock.Completed());
    episode.submit_at_start[i] = progress.submit_count;
    episode.resolved_tick_at_start[i] = progress.last_resolved_tick;
  }
  Trace("loading",
        "event=begin episode=%llu source=%s loop=%llu tick=%llu reasons=%u "
        "first_menu=%u queue=%llu,%llu generation=%llu,%llu overrides=%llu "
        "admitted=%llu session_generation=%llu a_generation=%llu b_generation=%llu "
        "submit=%llu,%llu resolved_tick=%llu,%llu",
        episode.id, episode.reason, r.loop, episode.start_completed,
        episode.reasons, episode.first_menu, episode.queue_at_start[0],
        episode.queue_at_start[1], episode.generation_at_start[0],
        episode.generation_at_start[1], episode.overrides_at_start,
        episode.admitted_at_start, SessionGeneration(r), r.players[0].Generation(),
        r.players[1].Generation(), episode.submit_at_start[0],
        episode.submit_at_start[1], episode.resolved_tick_at_start[0],
        episode.resolved_tick_at_start[1]);
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
  episode.admitted_at_end = r.peer.admitted;
  episode.queue_at_end = {r.players[0].Depth(), r.players[1].Depth()};
  episode.generation_at_end = {r.players[0].Generation(), r.players[1].Generation()};
  for (unsigned i = 0; i < r.session.ParticipantCount(); ++i) {
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
        r.players[0].Depth(), r.players[1].Depth(),
        episode.generation_at_start[0], episode.generation_at_start[1],
        r.players[0].Generation(), r.players[1].Generation(), SessionGeneration(r),
        episode.submit_at_start[0], episode.submit_at_start[1],
        episode.submit_at_end[0], episode.submit_at_end[1],
        episode.resolved_tick_at_start[0], episode.resolved_tick_at_start[1],
        episode.resolved_tick_at_end[0], episode.resolved_tick_at_end[1]);
}
void RecordLoadingQuery(bool nonzero, bool known_context) noexcept {
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
  // Hosting 未握手时也可保留菜单 Host；已握手时必须是 Warmup。
  const bool session_ready =
      !r->peer.established ||
      r->session.Status().state == core::SessionState::Warmup;
  const bool qualified =
      bit == StartOwner && r->mode == core::SessionMode::Host &&
      session_ready && !r->run_observed && !r->join.complete &&
      !RunActive() && r->device.State() == DeviceKState::Empty &&
      !r->players[0].BoundValid() && !r->players[1].BoundValid() &&
      !r->retiring && !r->failure;
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
        "session_generation=%llu episode=%llu a_generation=%llu b_generation=%llu",
        bit, unsigned(owner), OwnerToken(owner), unsigned(normal), r->loop,
        r->clock.Completed(), unsigned(r->topology[0].starting),
        r->topology[0].stage_transition, r->topology[0].room_transition,
        SessionGeneration(*r), SessionGeneration(*r),
        r->loading.active ? r->loading.id : 0,
        r->players[0].Generation(), r->players[1].Generation());
  if (!normal) {
    FailRuntime("loading_owner_exception");
    return;
  }
  // owner 正常返回后仍不能在同一 loop 作为稳定 Authority 入口。
  r->defer_through = (std::max)(r->defer_through, r->loop);
  if (bit == StartOwner)
    r->start_qualified = false;
}
LoadingOwnerScope::LoadingOwnerScope(bool start_owner) noexcept
    : bit_(start_owner ? StartOwner : LifecycleOwner), main_(IsMain()),
      exceptions_(std::uncaught_exceptions()) {
  if (main_)
    NoteLoadingOwnerEntered(bit_);
}
LoadingOwnerScope::~LoadingOwnerScope() noexcept {
  if (!main_)
    return;
  NoteLoadingOwnerReturned(bit_, exceptions_ == std::uncaught_exceptions());
}
void RefreshLifecycle(SessionRuntime &r, bool authority) noexcept {
  if (r.finished || r.retiring || r.join.scope ||
      (r.mode == core::SessionMode::Host && !r.peer.established))
    return;
  const bool host_mode = r.mode == core::SessionMode::Host;
  std::array<Topology, 2> values{ReadTopology(), {}};
  if (host_mode)
    values[1] = ReadTopology(1, r.device.Object());
  const bool remote = host_mode && r.join.complete;
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
  if (!value.known || !value.unique || !value.Alive() || !ContextHealthy())
    blocks |= Lifecycle;
  if (r.console_blocked ||
      (run && (value.pause > 0 || (!value.focused && value.pause_on_focus_lost))))
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
  if (remote && run && !loading) {
    const auto action = ObserveRemoteLife(r, value, values[1]);
    if (action == RemoteLifeAction::PeerLost) {
      // 先撤销 B 的输出资格，再请求有序退休；不重试 Join，不创建 K。
      InvalidateRemoteBinding(r, r.life.conflict_reason);
      RequestPeerLost(r, r.life.conflict_reason);
      return;
    }
  }
  // 4. A 的房间/生命/精确关联变化；B 的地址与归属变化由 ObserveRemoteLife
  // 处理，避免死亡状态被反复迭代成新的失效。
  if (r.observed && run && !loading) {
    if (!(values[0].room == r.topology[0].room))
      InvalidateBinding(r, "room_identity_changed", true);
    else if (!SameLife(values[0], r.topology[0]))
      InvalidateBinding(r, "player_lifecycle_changed", true);
    else if (!(values[0].pair == r.topology[0].pair) ||
             values[0].unique != r.topology[0].unique)
      InvalidateBinding(r, "exact_pair_changed", true);
  }
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
  if (r.loop <= r.defer_through)
    blocks |= Lifecycle;
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
  if (authority && !blocks) {
    if (!r.players[0].BoundValid() && values[0].unique && values[0].Alive()) {
      if (!r.players[0].Bind(values[0].pair)) {
        FailRuntime("unique_binding_rejected");
        return;
      }
      r.controlled_player[0] = values[0].pair.player;
      ++r.query_epoch;
      Trace("binding",
            "ordinal=0 bound=1 generation=%llu loop=%llu "
            "reason=stable_authority session_generation=%llu participant_generation=%llu epoch=%llu",
            r.players[0].Generation(), r.loop, SessionGeneration(r),
            r.players[0].Generation(), r.query_epoch);
    }
    if (remote && !r.players[1].BoundValid()) {
      const bool revived = r.life.status == RemoteLifeStatus::Revived;
      if (r.life.status == RemoteLifeStatus::Alive || revived)
        BindRevivedRemote(r, revived);
    }
  }
  ObservePersistenceWindows(r);
}
// Warmup 的稳定 Authority 只负责 K 的一次创建/等待 Join，不分配输入或推进
// tick。Active 之后只要求 A 与会话可用，允许 B 为 Lost/未绑定。
bool ReadyForAuthority(SessionRuntime &r) noexcept {
  RefreshLifecycle(r, true);
  if (r.retiring || r.blocks || !r.players[0].BoundValid())
    return false;
  if (r.mode == core::SessionMode::Host) {
    if (r.device.State() == DeviceKState::Empty) {
      if (!r.device.CreateOnce())
        FailRuntime("device_k_creation_failed");
      return false;
    }
    if (r.device.State() != DeviceKState::Registered) {
      FailRuntime("device_k_lost");
      return false;
    }
    if (r.session.Status().state == core::SessionState::Warmup &&
        (!r.join.complete || !r.players[1].BoundValid()))
      return false;
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
    // 只有真正的 Game::Exit 观察点才开启 run_exit 窗口。
    if (native_exit)
      OpenPersistenceWindow(PersistenceWindowKind::RunExit, "native_exit");
  }
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
  n("loading_admitted_current", r ? r->peer.admitted : 0);
  for (unsigned i = 0; i < 2; ++i) {
    const std::string prefix = std::string("loading.") + (i ? "b." : "a.");
    n((prefix + "queue_start").c_str(), episode ? episode->queue_at_start[i] : 0);
    n((prefix + "queue_end").c_str(), episode ? episode->queue_at_end[i] : 0);
    n((prefix + "generation_start").c_str(), episode ? episode->generation_at_start[i] : 0);
    n((prefix + "generation_end").c_str(), episode ? episode->generation_at_end[i] : 0);
    n((prefix + "submit_start").c_str(), episode ? episode->submit_at_start[i] : 0);
    n((prefix + "submit_end").c_str(), episode ? episode->submit_at_end[i] : 0);
    n((prefix + "resolved_tick_start").c_str(), episode ? episode->resolved_tick_at_start[i] : 0);
    n((prefix + "resolved_tick_end").c_str(), episode ? episode->resolved_tick_at_end[i] : 0);
  }
  out += "loading_source=" +
         std::string(episode ? episode->reason : "none") + "\n";
  out += "loading_end_reason=" +
         std::string(episode ? episode->end_reason : "none") + "\n";
  n("start_qualified", r && r->start_qualified);
  return out;
}
} // namespace cs::game
