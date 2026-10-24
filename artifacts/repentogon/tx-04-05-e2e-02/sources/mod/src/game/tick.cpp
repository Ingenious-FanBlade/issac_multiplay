#include "game/tick.h"
#include "game/context.h"
#include "game/input_a.h"
#include "game/lifecycle.h"
#include "game/state.h"
#include "runtime.h"
#include <bcrypt.h>
#include <cstdio>
#include <exception>
#include <limits>
#include <memory>

namespace cs::game {
namespace {
std::unique_ptr<SoloRuntime> solo;
uint64_t manager_loop = 0, generation = 0;
std::atomic<bool> armed_public{false}, foreign_seen{false};

void Reject(SoloRuntime &r, core::TickDecision reason,
            const char *detail) noexcept {
  r.last_decision = reason;
  Trace("tick_reject",
        "decision=%s reason=%s loop=%llu current=%llu generation=%llu",
        core::TickDecisionName(reason), detail, r.loop, r.clock.Current(),
        r.session.Identity().generation);
}
// Authority-pre 的唯一入口：先绑定与激活，再 drain、统一 Submit、Resolve
// current。
bool BeginTick(SoloRuntime &r, core::TickKind kind, uint64_t token) noexcept {
  if (!RuntimeHealthy() || InBlockedContext())
    return false;
  if (!ReadyForAuthority(r))
    return false;
  core::TickBoundary boundary{kind,
                              r.loop,
                              kind == core::TickKind::StandardRoom
                                  ? core::TickOwnerKind::Game
                                  : core::TickOwnerKind::OddNormal,
                              token,
                              GetCurrentThreadId(),
                              MainThread};
  auto decision = r.pipeline.BeginDrain(boundary, false);
  if (decision != core::TickDecision::Started) {
    FailRuntime("begin_drain_rejected", decision);
    return false;
  }
  if (!SubmitInput(r)) {
    FailRuntime("input_admission_failed");
    return false;
  }
  decision = r.pipeline.BeginSimulation();
  if (decision != core::TickDecision::Started) {
    FailRuntime("begin_simulation_rejected", decision);
    return false;
  }
  r.boundary = boundary;
  r.last_decision = decision;
  r.metrics = {};
  r.metrics.authority_pre = 1;
  r.metrics.input_sequence = r.players[0].CurrentSequence();
  r.tick_input = r.players[0].Current();
  Trace("tick_phase", "phase=pre tick=%llu loop=%llu owner_token=%llu generation=%llu owner=%s", r.clock.Current(), r.loop, token, r.session.Identity().generation,
        boundary.owner_kind == core::TickOwnerKind::Game ? "Game" : "OddNormal");
  return true;
}
void NativeReturn(SoloRuntime &r, bool normal) noexcept {
  if (!normal) {
    FailRuntime("abnormal_native_return", core::TickDecision::AbnormalReturn);
    return;
  }
  const auto decision = r.pipeline.NativeReturned(r.boundary, true);
  r.last_decision = decision;
  if (decision != core::TickDecision::Committed) {
    FailRuntime("native_return_rejected", decision);
  } else {
    ++r.metrics.native_commit;
    Trace("tick_phase", "phase=commit tick=%llu loop=%llu owner_token=%llu generation=%llu", r.clock.Current(), r.loop, r.boundary.scope_token, r.session.Identity().generation);
  }
}
// 一条 tick 事件汇总六种 consumer 的三类查询，避免逐查询填满日志队列。
void PublishTick(SoloRuntime &r) noexcept {
  char reads[900]{};
  size_t used = 0;
  for (unsigned i = 0; i < 6; ++i) {
    const auto &a = r.metrics.reads[i];
    const auto &b = r.metrics.nonzero[i];
    int n = _snprintf_s(
        reads + used, sizeof(reads) - used, _TRUNCATE,
        " c%u=%llu reads%u=%llu,%llu,%llu nonzero%u=%llu,%llu,%llu", i,
        r.metrics.consumers[i], i, a[0], a[1], a[2], i, b[0], b[1], b[2]);
    if (n < 0)
      break;
    used += size_t(n);
  }
  Trace("tick",
        "tick=%llu kind=%s loop=%llu owner_token=%llu generation=%llu "
        "a_generation=%llu "
        "move=%d,%d aim=%d,%d held=%u edge=%u player_update_entered=%llu "
        "player_native_reached=%llu "
        "player_skipped_by_chain=%llu shooting_suppressed=%llu fields_known=%u "
        "move_direction=%d "
        "velocity=%g,%g shoot_components=%g,%g shoot_flag=%u published=1 "
        "authority_pre=%llu native_commit=%llu owner=%s bomb_queries=%llu "
        "bomb_query_value=%g bomb_query_mismatches=%llu input_sequence=%llu%s",
        r.clock.Completed(),
        r.boundary.kind == core::TickKind::StandardRoom ? "StandardRoom"
                                                        : "OddPlayer",
        r.loop, r.boundary.scope_token, r.session.Identity().generation,
        r.players[0].Generation(), r.tick_input.move_x, r.tick_input.move_y,
        r.tick_input.aim_x, r.tick_input.aim_y, r.tick_input.held_flags,
        r.tick_input.edge_flags, r.metrics.player_entered,
        r.metrics.native_reached, r.metrics.chain_skipped,
        r.metrics.shooting_suppressed, r.metrics.player_fields_known,
        r.metrics.move_direction, r.metrics.velocity_x, r.metrics.velocity_y,
        r.metrics.shoot_x, r.metrics.shoot_y, unsigned(r.metrics.shoot_flag),
        r.metrics.authority_pre, r.metrics.native_commit,
        r.boundary.owner_kind == core::TickOwnerKind::Game ? "Game" : "OddNormal",
        r.metrics.bomb_queries, r.metrics.first_bomb_query,
        r.metrics.bomb_query_mismatches, r.metrics.input_sequence, reads);
}
} // namespace
SoloRuntime *Runtime() noexcept { return IsMain() ? solo.get() : nullptr; }
uint64_t ManagerLoop() noexcept { return IsMain() ? manager_loop : 0; }
bool SoloBusy() noexcept { return IsMain() && solo && !solo->finished; }
bool Armed() noexcept { return SoloBusy(); }
bool RuntimeHealthy() noexcept {
  if (!IsMain())
    return false;
  if (foreign_seen.exchange(false))
    FailRuntime("foreign_callback");
  if (TraceDropped())
    FailRuntime("trace_event_lost");
  auto *r = Runtime();
  return ContextHealthy() && r && !r->failure && !r->finished;
}
const char *StateName() noexcept {
  auto *r = Runtime();
  if (!r || r->finished)
    return "Idle";
  if (r->retiring)
    return "Retiring";
  return core::SessionStateName(r->session.Status().state);
}
void ForeignCall() noexcept {
  if (armed_public.load(std::memory_order_acquire))
    foreign_seen.store(true, std::memory_order_release);
}
// 会话只能由下一次 Manager 入口创建；身份使用系统随机源，重启不复位
// generation。
const char *StartSolo() noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager)
    return "wrong_boundary";
  if (SoloBusy())
    return "busy";
  if (!RunActive())
    return "not_in_run";
  if (!ContextHealthy() || PointerCount() != 8 || !OddPatchInstalled())
    return "adaptation_unavailable";
  if (generation == (std::numeric_limits<uint64_t>::max)())
    return "generation_exhausted";
  try {
    auto next = std::make_unique<SoloRuntime>();
    core::SessionIdentity identity;
    if (BCryptGenRandom(nullptr, identity.session.data(),
                        ULONG(identity.session.size()),
                        BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
      return "identity_random_failed";
    identity.generation = generation + 1;
    identity.clients[0] = 1;
    if (!next->session.Start(identity))
      return "session_identity_rejected";
    next->players[0].Initialize(identity.session, identity.clients[0], 0,
                                &next->session);
    next->loop = manager_loop;
    next->topology = ReadTopology();
    next->observed = next->topology.known;
    next->controlled_player = next->topology.pair.player;
    solo = std::move(next);
    generation = identity.generation;
    DiscardInputEdges();
    foreign_seen = false;
    armed_public = true;
    Trace("session",
          "from=Idle to=Warmup mode=Solo reason=Prepared generation=%llu "
          "loop=%llu",
          generation, manager_loop);
    return "";
  } catch (...) {
    return "session_allocation_failed";
  }
}
void StopSolo(core::SessionCause cause) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || r->retiring)
    return;
  const auto *before = core::SessionStateName(r->session.Status().state);
  r->session.BeginRetirement(cause);
  r->retiring = true;
  DiscardInputEdges();
  Trace("session", "from=%s to=Retiring reason=%s generation=%llu loop=%llu",
        before, core::SessionCauseName(cause), r->session.Identity().generation,
        r->loop);
}
void FailRuntime(const char *reason, core::TickDecision decision) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || r->failure)
    return;
  r->failure = true;
  r->retiring = true;
  r->last_error = reason;
  Reject(*r, decision, reason);
  r->pipeline.Abort(decision);
  r->session.Fail(core::SessionCause::RuntimeFailure);
  DiscardInputEdges();
  Trace("session",
        "to=Retiring core_state=Failed reason=%s generation=%llu loop=%llu",
        reason, r->session.Identity().generation, r->loop);
}
void BeginManager(uint64_t loop) noexcept {
  manager_loop = loop;
  BeginContextFrame();
  if (auto *r = Runtime()) {
    r->loop = loop;
    if (foreign_seen.exchange(false))
      FailRuntime("foreign_callback");
    if (!r->finished)
      RefreshLifecycle(*r, false);
  }
}
// NativeReturned 与 TopExit 分离；Disabled 的 post-state 仍必须提交 completed。
void EndManager(bool normal) noexcept {
  auto *r = Runtime();
  if (!r || r->finished)
    return;
  if (!normal)
    FailRuntime("manager_exception");
  if (normal &&
      r->pipeline.Status().phase == core::TickPipelinePhase::AwaitPostState) {
    auto result = r->pipeline.FinishPostState(r->loop);
    r->last_decision = result;
    if (result == core::TickDecision::Committed &&
        r->pipeline.PublishCompleted(TraceDropped() == 0)) {
      if (r->boundary.kind == core::TickKind::StandardRoom)
        ++r->ticks_standard;
      else
        ++r->ticks_odd;
      PublishTick(*r);
    } else
      FailRuntime("tick_publication_failed");
  }
  const auto end = r->pipeline.EndOuter(r->loop);
  if (end != core::TickDecision::Idle)
    FailRuntime("missing_native_return", end);
  if (r->retiring) {
    if (HasOwner(Owner::Odd) || HasOwner(Owner::Device) ||
        HasOwner(Owner::Game)) {
      ContextFault("retirement_callback_pending");
      return;
    }
    r->players[0].Retire();
    r->session.CompleteRetirement(true);
    r->finished = true;
    r->controlled_player = nullptr;
    r->blocks = 0;
    armed_public = false;
    Trace("session",
          "from=Retiring to=Idle reason=%s generation=%llu core_state=%s "
          "loop=%llu",
          r->failure ? r->last_error : "CleanupComplete",
          r->session.Identity().generation,
          core::SessionStateName(r->session.Status().state), r->loop);
  }
}
SimulationScope::SimulationScope(SimulationKind kind, void *object) noexcept
    : kind_(kind), main_(IsMain()), exceptions_(std::uncaught_exceptions()) {
  if (!main_ || !Armed())
    return;
  auto &r = *Runtime();
  if (kind == SimulationKind::Odd) {
    token_ = OwnerToken(Owner::Odd);
    if (r.odd_scope) {
      FailRuntime("odd_scope_reentry");
      return;
    }
    r.odd_scope = token_;
    r.odd_started = false;
  } else if (TopOwner() == Owner::Game && HasOwner(Owner::Manager)) {
    token_ = OwnerToken(Owner::Game);
    if (object != ReadTopology().room.room) {
      FailRuntime("room_owner_mismatch");
      return;
    }
    started_ = BeginTick(r, core::TickKind::StandardRoom, token_);
  } else {
    Trace("room_scope", "loop=%llu owner=%u owner_token=%llu eligible=0", r.loop, unsigned(TopOwner()), OwnerToken(TopOwner()));
  }
}
SimulationScope::~SimulationScope() noexcept {
  if (!main_ || !token_)
    return;
  auto *r = Runtime();
  if (!r || r->finished)
    return;
  const bool normal = exceptions_ == std::uncaught_exceptions();
  if (kind_ == SimulationKind::Odd) {
    ClearOddMarkers(token_);
    if (r->odd_scope != token_) {
      FailRuntime("odd_return_scope_mismatch");
      return;
    }
    started_ = r->odd_started;
    r->odd_scope = 0;
  }
  if (started_ &&
      r->pipeline.Status().phase == core::TickPipelinePhase::Simulating) {
    if (r->boundary.scope_token != token_) {
      FailRuntime("simulation_return_token_mismatch",
                  core::TickDecision::WrongReturn);
      return;
    }
    NativeReturn(*r, normal);
  } else if (!normal)
    FailRuntime("simulation_exception");
}
// 在普通 shim 标记对应的首个 A Player 入口开始 Odd tick，不看返回地址或桩帧。
bool BeginOddPlayer(void *player) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || r->retiring || !r->odd_scope ||
      !TakeOddNormal(player))
    return false;
  const auto topology = ReadTopology();
  if (topology.pair.player != player || r->odd_started)
    return false;
  if (BeginTick(*r, core::TickKind::OddPlayer, r->odd_scope)) {
    r->odd_started = true;
    return true;
  }
  return false;
}
std::string StatusFields() {
  auto *r = Runtime();
  std::string out = "session_state=" + std::string(StateName()) +
                    "\nsession_mode=" + (r ? "Solo" : "None") + "\n";
  const auto n = [&](const char *key, uint64_t value) {
    out += key;
    out += '=';
    out += std::to_string(value);
    out += '\n';
  };
  n("generation", r ? r->session.Identity().generation : 0);
  n("completed_tick", r ? r->clock.Completed() : 0);
  n("current_tick", r ? r->pipeline.SimulationTick() : 0);
  n("ticks_standard", r ? r->ticks_standard : 0);
  n("ticks_odd", r ? r->ticks_odd : 0);
  n("a_bound", r && !r->finished && r->players[0].BoundValid());
  n("a_generation", r ? r->players[0].Generation() : 0);
  n("blocks", r ? r->blocks : 0);
  n("queue_depth_a", r ? r->players[0].Depth() : 0);
  n("query_overrides", r ? r->overrides : 0);
  n("shooting_suppressed", r ? r->suppressed : 0);
  n("odd_patch_installed", OddPatchInstalled());
  out += "last_tick_decision=" +
         std::string(core::TickDecisionName(r ? r->last_decision
                                              : core::TickDecision::Idle)) +
         "\n";
  out += "session_error=" + std::string(r ? r->last_error : "") + "\n";
  return out;
}
} // namespace cs::game
