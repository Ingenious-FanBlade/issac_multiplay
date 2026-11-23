#include "game/observation/binary_entities.h"
#include "game/authority/tick.h"
#include "core/capture_cost.h"
#include "game/common/context.h"
#include "game/input/input_a.h"
#include "game/participants/join.h"
#include "game/authority/lifecycle.h"
#include "game/participants/peer.h"
#include "game/authority/persistence.h"
#include "game/participants/remote_life.h"
#include "game/authority/state.h"
#include "game/observation/world_bridge.h"
#include "game/mods/mod_api.h"
#include "runtime.h"
#include "net/timing_clock.h"
#include <algorithm>
#include <bcrypt.h>
#include <psapi.h>
#include <cstdio>
#include <exception>
#include <limits>
#include <memory>

namespace cs::game {
namespace {
std::unique_ptr<SessionRuntime> solo;
uint64_t manager_loop = 0, generation = 0;
std::atomic<bool> armed_public{false}, foreign_seen{false};

void Reject(SessionRuntime &r, core::TickDecision reason,
            const char *detail) noexcept {
  r.last_decision = reason;
  Trace("tick_reject",
        "decision=%s reason=%s loop=%llu current=%llu generation=%llu",
        core::TickDecisionName(reason), detail, r.loop, r.clock.Current(),
        SessionGeneration(r));
}
// Authority-pre 的唯一入口：先绑定与激活，再 drain、统一 Submit、Resolve
// current。
bool BeginTick(SessionRuntime &r, core::TickKind kind,
               uint64_t token) noexcept {
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
  for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i) {
    r.metrics[i] = {};
    r.metrics[i].authority_pre = 1;
    r.metrics[i].input_sequence = r.players[i].CurrentSequence();
    // 以实际 Authority tick 起算，consumer 缺席也不能延长恢复窗口。
    auto &recovery = r.shooting_recovery[i];
    if (!r.players[i].BoundValid())
      recovery = {};
    else if (recovery.generation != r.players[i].Generation())
      recovery = {r.players[i].Generation(), r.clock.Current(), false};
    if (r.players[i].BoundValid() && r.metrics[i].input_sequence)
      recovery.source_seen = true;
    r.metrics[i].input_generation = r.players[i].Generation();
    r.metrics[i].input_epoch = r.query_epoch;
    r.tick_input[i] = r.players[i].Current();
  }
  Trace(
      "tick_phase",
      "phase=pre tick=%llu loop=%llu owner_token=%llu generation=%llu owner=%s",
      r.clock.Current(), r.loop, token, r.session.Identity().generation,
      boundary.owner_kind == core::TickOwnerKind::Game ? "Game" : "OddNormal");
  return true;
}
void NativeReturn(SessionRuntime &r, bool normal) noexcept {
  if (!normal) {
    FailRuntime("abnormal_native_return", core::TickDecision::AbnormalReturn);
    return;
  }
  const auto decision = r.pipeline.NativeReturned(r.boundary, true);
  r.last_decision = decision;
  if (decision != core::TickDecision::Committed) {
    FailRuntime("native_return_rejected", decision);
  } else {
    WorldNativeReturned(r.boundary, r.clock.Current());
    for (unsigned i = 0; i < r.session.ParticipantLimit(); ++i)
      ++r.metrics[i].native_commit;
    Trace("tick_phase",
          "phase=commit tick=%llu loop=%llu owner_token=%llu generation=%llu",
          r.clock.Current(), r.loop, r.boundary.scope_token,
          r.session.Identity().generation);
  }
}
// 一条 tick 事件汇总六种 consumer 的三类查询，避免逐查询填满日志队列。
void PublishTick(SessionRuntime &r, unsigned ordinal) noexcept {
  if (!Settings.diagnostics)
    return;
  char reads[900]{};
  size_t used = 0;
  for (unsigned i = 0; i < 6; ++i) {
    const auto &a = r.metrics[ordinal].reads[i];
    const auto &b = r.metrics[ordinal].nonzero[i];
    int n =
        _snprintf_s(reads + used, sizeof(reads) - used, _TRUNCATE,
                    " c%u=%llu reads%u=%llu,%llu,%llu nonzero%u=%llu,%llu,%llu",
                    i, r.metrics[ordinal].consumers[i], i, a[0], a[1], a[2], i,
                    b[0], b[1], b[2]);
    if (n < 0)
      break;
    used += size_t(n);
  }
  Trace(
      "tick",
      "ordinal=%u tick=%llu kind=%s loop=%llu owner_token=%llu generation=%llu "
      "participant_generation=%llu a_generation=%llu b_generation=%llu "
      "session_generation=%llu input_generation=%llu input_epoch=%llu "
      "query_epoch=%llu b_life=%s b_bound=%u death=%llu "
      "move=%d,%d aim=%d,%d held=%u edge=%u player_update_entered=%llu "
      "player_native_reached=%llu "
      "player_skipped_by_chain=%llu shooting_suppressed=%llu "
      "shooting_startup_deferred=%llu fields_known=%u "
      "move_direction=%d "
      "velocity=%g,%g shoot_components=%g,%g shoot_flag=%u published=1 "
      "authority_pre=%llu native_commit=%llu owner=%s bomb_queries=%llu "
      "bomb_query_value=%g bomb_query_mismatches=%llu input_sequence=%llu "
      "query_reads=%llu query_repeats=%llu query_mismatches=%llu "
      "query_read_mask=%x query_repeat_mask=%x query_mismatch_mask=%x%s",
      ordinal, r.clock.Completed(),
      r.boundary.kind == core::TickKind::StandardRoom ? "StandardRoom"
                                                      : "OddPlayer",
      r.loop, r.boundary.scope_token, r.session.Identity().generation,
      r.players[ordinal].Generation(), r.players[0].Generation(),
      r.mode == core::SessionMode::Host ? r.players[1].Generation() : 0,
      SessionGeneration(r), r.metrics[ordinal].input_generation,
      r.metrics[ordinal].input_epoch, r.query_epoch,
      RemoteLifeStatusName(r.life.status),
      unsigned(r.mode == core::SessionMode::Host && r.players[1].BoundValid()),
      r.life.death_event, r.tick_input[ordinal].move_x,
      r.tick_input[ordinal].move_y, r.tick_input[ordinal].aim_x,
      r.tick_input[ordinal].aim_y, r.tick_input[ordinal].held_flags,
      r.tick_input[ordinal].edge_flags, r.metrics[ordinal].player_entered,
      r.metrics[ordinal].native_reached, r.metrics[ordinal].chain_skipped,
      r.metrics[ordinal].shooting_suppressed,
      r.metrics[ordinal].shooting_startup_deferred,
      r.metrics[ordinal].player_fields_known, r.metrics[ordinal].move_direction,
      r.metrics[ordinal].velocity_x, r.metrics[ordinal].velocity_y,
      r.metrics[ordinal].shoot_x, r.metrics[ordinal].shoot_y,
      unsigned(r.metrics[ordinal].shoot_flag), r.metrics[ordinal].authority_pre,
      r.metrics[ordinal].native_commit,
      r.boundary.owner_kind == core::TickOwnerKind::Game ? "Game" : "OddNormal",
      r.metrics[ordinal].bomb_queries, r.metrics[ordinal].first_bomb_query,
      r.metrics[ordinal].bomb_query_mismatches,
      r.metrics[ordinal].input_sequence, r.metrics[ordinal].query_reads,
      r.metrics[ordinal].query_repeats, r.metrics[ordinal].query_mismatches,
      r.metrics[ordinal].query_read_mask, r.metrics[ordinal].query_repeat_mask,
      r.metrics[ordinal].query_mismatch_mask, reads);
}
} // namespace
SessionRuntime *Runtime() noexcept { return IsMain() ? solo.get() : nullptr; }
uint64_t ManagerLoop() noexcept { return IsMain() ? manager_loop : 0; }
bool Busy() noexcept { return IsMain() && solo && !solo->finished; }
bool SoloBusy() noexcept {
  return Busy() && solo->mode == core::SessionMode::Solo;
}
bool HostBusy() noexcept {
  return Busy() && solo->mode == core::SessionMode::Host;
}
std::uint64_t LiveCaptureSession() noexcept {
  auto *r=Runtime();if(!r||r->mode!=core::SessionMode::Host||r->finished||r->retiring||r->failure)return 0;
  // 大厅阶段先准备生命周期采集。没有 run 时不会采集或发布游戏世界。
  std::uint64_t session=0;std::memcpy(&session,r->session.Identity().session.data(),sizeof(session));return session?session:1;
}
// 属性栏使用 Authority 的本地控制组，包括该组下的多个身体。
bool HostOwnsFoundHudPlayer(const void *player) noexcept {
  const auto *r=Runtime();
  return r&&r->topology[0].Contains(const_cast<void *>(player),false);
}
// 身体属于哪个参与者（0 为房主，含多身体与非名册副身体）；不属于任何已知控制组时返回 false。
// 只读当前拓扑，供绘制时区分本端与他人身体（描边／透明度），不参与输入或同步判定。
bool HostPlayerParticipant(const void *player,unsigned &ordinal) noexcept {
  const auto *r=Runtime();
  if(!r||!player||r->mode!=core::SessionMode::Host||r->finished)return false;
  for(unsigned i=0;i<r->topology.size();++i)
    if(r->topology[i].Contains(const_cast<void *>(player),false)){ordinal=i;return true;}
  return false;
}
bool HostModIntentContext(net::ModIntent &intent) noexcept {
  const auto *r=Runtime();
  const auto capture=CurrentWorldRecording();
  if(!r||r->mode!=core::SessionMode::Host||r->failure||r->retiring||r->finished||r->run_torn_down||
     r->control_lifecycle_depth||InBlockedContext()||!capture||!capture->binary_entities||
     !capture->binary_entities->ModIntentWorld(intent))return false;
  intent.connection=SessionGeneration(*r);intent.ordinal=0;
  return true;
}
bool SubmitHostModIntent(const net::ModIntent &intent) noexcept {
  auto *r=Runtime();
  return r&&HostBusy()&&!r->retiring&&!r->finished&&r->network.QueueLocalModIntent(intent);
}
std::uint16_t HostModIntentMember(const void *object) noexcept {
  const auto capture=CurrentWorldRecording();
  return capture&&capture->binary_entities?capture->binary_entities->ModIntentMember(object):state::kModGlobal;
}
void *ResolveHostModIntentMember(const net::ModIntent &intent,std::uint16_t number) noexcept {
  const auto capture=CurrentWorldRecording();
  return capture&&capture->binary_entities?capture->binary_entities->ResolveModIntentMember(intent,number):nullptr;
}
// 相机只使用当前房间内已确认属于房主的身体，绑定撤销时回到原生跟随。
bool HostOwnsCameraPlayer(const void *room,const void *player) noexcept {
  const auto *r=Runtime();
  if(!r||r->mode!=core::SessionMode::Host||r->finished||r->retiring||r->failure||r->control_lifecycle_depth)return false;
  const auto &group=r->topology[0];
  if(!group.known||!group.run||!group.unique||group.room.room!=room)return false;
  for(unsigned i=0;i<group.body_count;++i)
    if(group.bodies[i].player==player)return group.bodies[i].active;
  return false;
}
bool Armed() noexcept { return Busy() && solo->core_started; }
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
  if (r->mode == core::SessionMode::Host && !r->core_started)
    return "Hosting";
  return core::SessionStateName(r->session.Status().state);
}
void ForeignCall() noexcept {
  if (armed_public.load(std::memory_order_acquire))
    foreign_seen.store(true, std::memory_order_release);
}
bool ForeignCallPending() noexcept {
  return foreign_seen.load(std::memory_order_acquire);
}
// 会话只能由下一次 Manager 入口创建；身份使用系统随机源，重启不复位
// generation。
const char *StartSolo() noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager)
    return "wrong_boundary";
  if (Busy())
    return "busy";
  if (!RunActive())
    return "not_in_run";
  if (!ContextHealthy() || !Capabilities[size_t(Group::Observe)] ||
      PointerCount() != 8 || !OddPatchInstalled())
    return "adaptation_unavailable";
  if (generation == (std::numeric_limits<uint64_t>::max)())
    return "generation_exhausted";
  try {
    auto next = std::make_unique<SessionRuntime>();
    next->mode = core::SessionMode::Solo;
    next->core_started = true;
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
    next->topology[0] = ReadTopology();
    next->observed = next->topology[0].known;
    next->controlled_player[0] = next->topology[0].pair.player;
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
// 主菜单也可监听；只有传输握手成功后才准备 core 并接管 A。
const char *StartHost(const net::WorkerConfig &config, int character) noexcept {
  if (!IsMain() || TopOwner() != Owner::Manager)
    return "wrong_boundary";
  if (Busy())
    return "busy";
  if (config.lobby && !LobbyCanListen())
    return "lobby_requires_main_menu";
  if (!ContextHealthy() || !Capabilities[size_t(Group::Observe)] ||
      PointerCount() != 8 || !OddPatchInstalled() || !HostPatchesInstalled() ||
      !Capabilities[size_t(Group::Join)])
    return "adaptation_unavailable";
  if (generation == (std::numeric_limits<uint64_t>::max)())
    return "generation_exhausted";
  try {
    auto next = std::make_unique<SessionRuntime>();
    next->mode = core::SessionMode::Host;
    next->join.character = character;
    next->peer.session_generation = ++generation;
    next->loop = manager_loop;
    auto network_config=config;
    if (const auto *error=CaptureModManifest(network_config.mods); *error)
      return error;
    auto started = next->network.Start(network_config);
    if (!started)
      return net::StartErrorName(started.error);
    if (config.lobby) {
      next->lobby.roster = {core::LobbyPhase::Waiting, 1};
      next->lobby.addresses = std::move(started.addresses);
      next->lobby.port = config.port;
    }
    for (auto &remote : next->remotes) {
      remote.peer.identity = started.identity;
      remote.peer.session_generation = generation;
      remote.join.character = character;
    }
    core::SessionIdentity identity{started.identity.session,
                                   started.identity.clients, generation};
    if (!next->session.Start(identity, core::SessionMode::Host))
      return "host_identity_rejected";
    for (unsigned i = 0; i < core::kMaximumParticipants; ++i)
      next->players[i].Initialize(identity.session, identity.clients[i], i,
                                  &next->session);
    next->core_started = true;
    solo = std::move(next);
    foreign_seen = false;
    Trace("session",
          "from=Idle to=Hosting mode=Host generation=%llu port=%u role=%d",
          generation, config.port, character);
    return "";
  } catch (...) {
    return "session_allocation_failed";
  }
}
void StopSolo(core::SessionCause cause) noexcept {
  if (SoloBusy())
    StopSession(cause);
}
// Mod 卸载在 Lua/控制台栈上只记标记，此处沿用 Host 的主线程退休路径。
void RejectHostModReload() noexcept {
  FailRuntime("mod_lua_reloaded");
}
void StopSession(core::SessionCause cause) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || r->retiring)
    return;
  const auto *before = StateName();
  if (r->mode == core::SessionMode::Host && !r->core_started) {
    // Hosting 尚未启动 core，也要退休 worker；不伪造 Warmup/绑定。
  } else if (!r->session.BeginRetirement(cause))
    return;
  r->retirement_cause = cause;
  r->retiring = true;
  for (unsigned i = 0; i < core::kMaximumParticipants; ++i)
    WorldRevokePlayer(i);
  DiscardInputEdges();
  Trace("session", "from=%s to=Retiring reason=%s generation=%llu loop=%llu",
        before, core::SessionCauseName(cause), SessionGeneration(*r), r->loop);
}
void FailRuntime(const char *reason, core::TickDecision decision) noexcept {
  auto *r = Runtime();
  if (!r || r->finished || r->failure)
    return;
  r->failure = true;
  for (unsigned i = 0; i < core::kMaximumParticipants; ++i)
    WorldRevokePlayer(i);
  r->retiring = true;
  r->last_error = reason;
  if (r->retirement_cause == core::SessionCause::None)
    r->retirement_cause = core::SessionCause::RuntimeFailure;
  Reject(*r, decision, reason);
  r->pipeline.Abort(decision);
  r->session.Fail(core::SessionCause::RuntimeFailure);
  DiscardInputEdges();
  Trace("session",
        "to=Retiring core_state=Failed reason=%s generation=%llu loop=%llu",
        reason, SessionGeneration(*r), r->loop);
}
void BeginManager(uint64_t loop) noexcept {
  manager_loop = loop;
  BeginContextFrame();
  if (auto *r = Runtime()) {
    r->loop = loop;
    if (foreign_seen.exchange(false))
      FailRuntime("foreign_callback");
    if (!r->finished) {
      PollPeer(*r);
      RefreshHostLobby(*r);
      armed_public = Armed();
      if (Armed())
        RefreshLifecycle(*r, false);
      DrainPeer(*r);
    }
    if(r->mode==core::SessionMode::Host){
      net::ModIntentInbox::Batch pending;
      const auto count=r->network.TakeModIntents(pending);
      for(unsigned i=0;i<count;++i){
        const auto &intent=pending[i];
        net::ModIntentResult result;
        result.id=intent.id;result.connection=intent.connection;result.run=intent.run;result.floor=intent.floor;
        result.mod=intent.mod;result.declaration=intent.declaration;
        net::ModIntent current;
        if(intent.ordinal<4&&HostModIntentContext(current)&&intent.run==current.run&&intent.floor==current.floor&&
           (intent.ordinal?(!r->Remote(intent.ordinal).retiring&&!r->Remote(intent.ordinal).retired):
             intent.connection==current.connection)){
          const auto &group=r->topology[intent.ordinal];
          if(group.known&&group.unique)for(unsigned n=0;n<group.body_count;++n)
            if(group.bodies[n].active&&group.bodies[n].player){RunModIntent(intent,group.bodies[n].player,result);break;}
        }
        if(intent.ordinal){
          if(!r->network.QueueModResult(intent.ordinal,result))Trace("mod_state","phase=intent_result_queue");
        }else DispatchModIntentResult(result,false);
      }
    }
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
    const auto post = WorldPostState(r->clock.Current());
    auto result = r->pipeline.FinishPostState(r->loop, post);
    // 世界 publication 只由 clock 提交决定，文件/通知失败不参与输入 Ack 成败。
    WorldPublishPostState(result == core::TickDecision::Committed,
                          r->pipeline.Receipt().post);
    r->last_decision = result;
    if (result == core::TickDecision::Committed &&
        r->pipeline.PublishCompleted(!Settings.diagnostics ||
                                     TraceDropped() == 0)) {
      r->committed_sample_ms=cs::net::TimingClockMs();  // 输入时序链用高精度钟（net/timing_clock.h）
      r->committed_kind=r->boundary.kind==core::TickKind::OddPlayer?1u:0u;
      if (r->boundary.kind == core::TickKind::StandardRoom)
        ++r->ticks_standard;
      else
        ++r->ticks_odd;
      for (unsigned i = 0; i < r->session.ParticipantLimit(); ++i)
        if (r->session.HasParticipant(i))
          PublishTick(*r, i);
    } else
      FailRuntime("tick_publication_failed");
  }
  const auto end = r->pipeline.EndOuter(r->loop);
  if (end != core::TickDecision::Idle)
    FailRuntime("missing_native_return", end);
  if (normal)
    PublishPeer(*r);
  if (!r->retiring)
    FinishRemoteRetirements(*r);
  if (r->retiring && !r->cleanup_failed) {
    if (HasOwner(Owner::Odd) || HasOwner(Owner::Device) ||
        HasOwner(Owner::Join) || HasOwner(Owner::Input) ||
        HasOwner(Owner::Game) || r->control_lifecycle_depth) {
      ContextFault("retirement_callback_pending");
      return;
    }
    FinishRemoteRetirements(*r);
    bool device_ok = true, player_ok = true;
    if (r->mode == core::SessionMode::Host)
      for (const auto &remote : r->remotes) {
        device_ok &= !remote.cleanup_failed;
        player_ok &= remote.retired;
      }
    const auto peer_stop = PollPeerStop(*r);
    if (!device_ok || !player_ok || peer_stop == net::StopResult::WaitFailed) {
      r->cleanup_failed = true;
      EndLoadingEpisode(*r, "retirement_incomplete");
      FinishPersistenceWindows("retirement_incomplete");
      // 清理未完成也要输出已收集的查询覆盖，避免证据随 Retiring 丢失。
      PublishQueryCoverage(*r);
      Trace("retirement_error", "device=%u player=%u peer=%u peer_wait_failed=%u loop=%llu",
            device_ok, player_ok, peer_stop == net::StopResult::Stopped,
            peer_stop == net::StopResult::WaitFailed, r->loop);
      return; // 保留未知现场及常驻对象，不重复原生移除，不把失败报告为 Idle。
    }
    if (peer_stop != net::StopResult::Stopped) return;
    for (unsigned i = 0; i < r->session.ParticipantLimit(); ++i)
      r->players[i].Retire();
    if (r->core_started)
      r->session.CompleteRetirement(true);
    r->finished = true;
    // 退休完成后 join_state 复位为 None，最终业务结果保留在 join_last。
    for (unsigned i = 1; i < core::kMaximumParticipants; ++i)
      ResetJoinStateAfterRetirement(*r, i);
    if (r->mode == core::SessionMode::Host)
      Trace("life",
            "event=retired status=None last_status=%s deaths=%llu "
            "revives=%llu rebinds=%llu session_generation=%llu "
            "participant_generation=%llu join_state=%s join_last=%s loop=%llu",
            RemoteLifeStatusName(r->life.status), r->life.deaths,
            r->life.revives, r->life.rebinds, SessionGeneration(*r),
            r->players[1].Generation(), JoinStateName(r->join.state),
            JoinStateName(r->join.last), r->loop);
    // 未收口的持久窗口按残留记录，不把未观测当成“无副作用”。
    EndLoadingEpisode(*r, "session_retired");
    FinishPersistenceWindows("session_retired");
    // 分键查询覆盖在会话结束时输出一次，供 V13 逐键归约。
    PublishQueryCoverage(*r);
    r->controlled_player.fill(nullptr);
    r->blocks = 0;
    armed_public = false;
    Trace("session",
          "from=Retiring to=Idle reason=%s generation=%llu core_state=%s "
          "loop=%llu",
          r->failure ? r->last_error
                     : core::SessionCauseName(r->retirement_cause),
          SessionGeneration(*r),
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
    Trace("room_scope", "loop=%llu owner=%u owner_token=%llu eligible=0",
          r.loop, unsigned(TopOwner()), OwnerToken(TopOwner()));
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
// 详细聚合按页显式读取，网络值已复制；不跨线程借用任何游戏对象。
std::string HostStreamFields(unsigned ordinal,unsigned page){
  const auto *r=Runtime();if(!IsMain()||!r||ordinal<1||ordinal>3||page>=32)return "ok=0\nerror=stream_peer_unavailable\n";
  const auto status=r->network.Status(ordinal);std::string out="ok=1\nstream_diagnostics_version=1\n";
  out+=Settings.stream.Fields("stream.requested.");out+=status.stream_config.Fields("stream.effective.");out+=replica::stream::LimitsFields(status.stream_limits,"stream.negotiated.");out+=replica::stream::LimitsFields(status.stream_fast_limits,"stream.negotiated_fast.");
  out+=status.stream_metrics.Fields("stream.publication.",page);out+=status.encoder_metrics.Fields("stream.shared_encoder.",page);
  const auto add=[&](const char *name,std::uint64_t value){out+="stream."+std::string(name)+"="+std::to_string(value)+"\n";};
  add("peer",ordinal);add("packets",status.uplink_packets);add("udp_payload_bytes",status.uplink_payload_bytes);add("wire_bytes",status.uplink_wire_bytes);add("bundle_messages",status.uplink_bundle_messages);add("budget_blocks",status.uplink_budget_blocks);add("stale",status.stream_stale);add("phase",status.publication_phase);
  const auto &t=status.stream_transfer;add("fragments",t.fragments);add("retransmitted_fragments",t.retransmitted);add("expired_transfers",t.expired);
  for(unsigned i=0;i<4;++i)out+="stream.lane."+std::to_string(i)+".wire_bytes="+std::to_string(status.uplink_lane_bytes[i])+"\nstream.lane."+std::to_string(i)+".packet_incidence="+std::to_string(status.uplink_lane_packets[i])+"\n";
  out+="stream.first_failure="+std::string(status.replication_first_failure.data())+"\n";
  PROCESS_MEMORY_COUNTERS_EX pm{};pm.cb=sizeof(pm);if(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pm),sizeof(pm))){add("process_private_bytes",pm.PrivateUsage);add("process_working_set_bytes",pm.WorkingSetSize);}else out+="stream.process_memory=unavailable\n";
  for(unsigned i=0;i<3;++i){const auto &p=status.replication_memory[i];out+="stream.pool."+std::to_string(i)+"="+std::to_string(p.current)+"/"+std::to_string(p.high_water)+"/"+std::to_string(p.limit)+"\n";}
  out+=memory::ReplicationBudget()->FailureFields();
  return out;
}
std::string SourceStreamFields(unsigned page){
  if(!IsMain()||page>=32)return "ok=0\nerror=stream_source_arguments\n";
  const auto c=CurrentWorldRecording();if(!c)return "ok=0\nerror=stream_source_off\n";
  const auto *source=c->binary_entities.get();
  if(!source)return "ok=0\nerror=binary_source_off\n";
  const auto stats=source->Statistics();
  auto out="ok=1\nstream.source.ready="+std::to_string(stats.active)+
      "\nstream.source.sample_ms="+std::to_string(GetTickCount64())+
      "\nstream.source.ordinary="+std::to_string(stats.ordinary)+
      "\nstream.source.bodies="+std::to_string(stats.bodies)+
      "\nstream.source.gaps="+std::to_string(stats.gaps)+
      "\nstream.source.object_failures="+std::to_string(stats.object_failures)+
      "\nstream.source.body_skipped="+std::to_string(stats.body_skipped)+
      "\nstream.source.retained_roots="+std::to_string(stats.retained_roots)+
      "\nstream.source.max_retained_frames="+std::to_string(stats.max_retained_frames)+
      "\nstream.source.first_object_failure.reason="+std::to_string(unsigned(stats.first_object_failure.reason))+
      "\nstream.source.first_object_failure.space="+std::to_string(stats.first_object_failure.subject.space)+
      "\nstream.source.first_object_failure.id="+std::to_string(stats.first_object_failure.subject.local)+
      "\nstream.source.first_object_failure.use="+std::to_string(stats.first_object_failure.subject.use)+
      "\nstream.source.first_object_failure.layout="+std::to_string(stats.first_object_failure.layout)+
      "\nstream.source.first_object_failure.field="+std::to_string(stats.first_object_failure.field)+
      "\nstream.source.first_object_failure.detail="+stats.first_object_failure.detail.data()+
      "\nstream.source.member_indices.high_water="+std::to_string(stats.member_indices.high_water)+
      "\nstream.source.member_indices.reused="+std::to_string(stats.member_indices.reused)+
      "\nstream.source.transient_entity_retirements="+std::to_string(stats.transient_entity_retirements)+
      "\nstream.source.capture_failure="+std::to_string(unsigned(stats.failure.reason))+
      "\nstream.source.capture_layout="+std::to_string(stats.failure.layout)+
      "\nstream.source.capture_field="+std::to_string(stats.failure.field)+
      "\nstream.source.capture_origin="+stats.failure.origin.file_name()+":"+std::to_string(stats.failure.origin.line())+
      "\nstream.source.capture_detail="+stats.failure.detail.data()+
      "\nstream.source.read_copies="+std::to_string(stats.reads.copies)+
      "\nstream.source.snapshot_reads="+std::to_string(stats.native.snapshot_reads)+
      "\nstream.source.snapshot_fallbacks="+std::to_string(stats.native.snapshot_fallbacks)+
      "\nstream.source.read_bytes="+std::to_string(stats.reads.bytes)+
      "\nstream.source.read_queries="+std::to_string(stats.reads.queries)+
      "\nstream.source.read_hits="+std::to_string(stats.reads.hits)+
      "\nstream.source.read_fallbacks="+std::to_string(stats.reads.fallbacks)+
      "\nstream.source.read_failures="+std::to_string(stats.reads.failures)+
      "\nstream.source.root_resource_checks="+std::to_string(stats.reads.root_resource_checks)+
      "\nstream.source.root_resource_revocations="+std::to_string(stats.reads.root_resource_revocations)+
      "\nstream.source.string_blocks="+std::to_string(stats.reads.string_blocks)+
      "\nstream.source.body_reused="+std::to_string(stats.body_reused)+
      "\nstream.source.member_scans="+std::to_string(stats.member_scans)+
      "\nstream.source.scene_scans="+std::to_string(stats.scene_scans)+
      "\nstream.source.topology_builds="+std::to_string(stats.native.topology_builds)+
      "\nstream.source.topology_hits="+std::to_string(stats.native.topology_hits)+
      "\nstream.source.shared_player_prepared="+std::to_string(stats.native.shared_player_prepared)+
      "\nstream.source.shared_player_consumed="+std::to_string(stats.native.shared_player_consumed)+
      "\nstream.source.shared_player_reviews="+std::to_string(stats.native.shared_player_reviews)+
      "\nstream.source.witnesses="+std::to_string(stats.native.witnesses)+
      "\nstream.source.witness_rechecks="+std::to_string(stats.native.rechecks)+
      "\nstream.source.source_changed="+std::to_string(stats.source_changed)+
      "\nstream.source.legacy_attempts="+std::to_string(c->collector?c->collector->LegacyAttempts():0)+
      "\nstream.source.replaced="+std::to_string(stats.exchange.replaced)+
      "\nstream.source.revoked="+std::to_string(stats.exchange.revoked)+
      "\nstream.source.roots="+std::to_string(stats.roots)+
      "\nstream.source.cells="+std::to_string(stats.cells)+
      "\nstream.source.paths="+std::to_string(stats.paths)+
      "\nstream.source.bytes="+std::to_string(stats.bytes)+
      "\nstream.source.revision="+std::to_string(stats.publication_revision)+
      "\nstream.source.reason="+stats.publication_failure.data()+"\n";
  constexpr const char *categories[]={"file","selected_file","image","item","animation","image_path","config"};
  out+="stream.source.unchanged.checks="+std::to_string(stats.native.unchanged_checks)+
    "\nstream.source.unchanged.roots="+std::to_string(stats.native.unchanged_roots)+
    "\nstream.source.unchanged.unavailable="+std::to_string(stats.native.unchanged_unavailable)+
    "\nstream.source.unchanged.slots="+std::to_string(stats.native.unchanged_slots)+
    "\nstream.source.resource.frame_slots="+std::to_string(stats.native.resource_frame_slots)+"\n";
  for(unsigned i=0;i<stats.native.resource_hits.size();++i){const auto key=std::string("stream.source.resource.")+categories[i];
    out+=key+".parses="+std::to_string(stats.native.resource_parses[i])+"\n";
    out+=key+".hits="+std::to_string(stats.native.resource_hits[i])+"\n";
    out+=key+".frame_hits="+std::to_string(stats.native.resource_frame_hits[i])+"\n";
    out+=key+".instance_hits="+std::to_string(stats.native.resource_instance_hits[i])+"\n";
    out+=key+".shared_hits="+std::to_string(stats.native.resource_shared_hits[i])+"\n";
  }
  constexpr const char *layers[]={"sprite","layer","animation","image","config"};
  for(unsigned i=0;i<stats.native.resource_cache_capacity.size();++i){
    const auto key=std::string("stream.source.resource_cache.")+layers[i];
    out+=key+".capacity="+std::to_string(stats.native.resource_cache_capacity[i])+"\n";
    out+=key+".high_water="+std::to_string(stats.native.resource_cache_high_water[i])+"\n";
  }
  if(page==0){
    out+=ModStateFields();
    out+="mod.encode.missing="+std::to_string(stats.mod_missing_references)+"\n";
    out+="mod.encode.failures="+std::to_string(stats.mod_encode_failures)+"\n";
  }
  return out+source->WorkerFields(page);
}
std::string StatusFields() {
  auto *r = Runtime();
  std::string out =
      "session_state=" + std::string(StateName()) +
      "\nsession_mode=" + (r ? core::SessionModeName(r->mode) : "None") + "\n";
  const auto n = [&](const char *key, uint64_t value) {
    out += key;
    out += '=';
    out += std::to_string(value);
    out += '\n';
  };
  n("participants", r ? r->session.ParticipantCount() : 0);
  // 公开随机会话的纯值编号用于真实端身份关联，不公开对象地址或文件摘要。
  std::uint64_t wire_session=0;
  if(r)std::memcpy(&wire_session,r->session.Identity().session.data(),sizeof(wire_session));
  n("host.wire_session",wire_session?wire_session:1);
  n("participant_capacity", core::kMaximumParticipants);
  unsigned body_total = 0;
  if (r)
    for (unsigned i = 0; i < r->session.ParticipantLimit(); ++i) {
      const std::string prefix = "participant." + std::to_string(i) + ".";
      const auto number = [&](const char *key, uint64_t value) {
        n((prefix + key).c_str(), value);
      };
      number("controller", r->topology[i].pair.key);
      number("present", r->session.HasParticipant(i));
      number("bound", r->players[i].BoundValid());
      number("quarantined", r->control_quarantined[i]);
      number("generation", r->players[i].Generation());
      number("queue", r->players[i].Depth());
      const auto progress = r->players[i].Progress(r->clock.Completed());
      number("accepted_sequence", progress.last_accepted_sequence);
      number("processed_sequence", progress.last_processed_sequence);
      number("input_source", progress.last_processed_source_sequence);
      for (unsigned action = 0; action < progress.edges.size(); ++action) {
        const auto key = prefix + "edge." + std::to_string(action) + ".";
        n((key + "processed_count").c_str(),
          progress.edges[action].processed_count);
        n((key + "processed_sequence").c_str(),
          progress.edges[action].processed_sequence);
      }
      number("bodies", r->topology[i].body_count);
      number("active_bodies", r->topology[i].active_bodies);
      if (r->session.HasParticipant(i))
        body_total += r->topology[i].active_bodies;
      if (i) {
        const auto &remote = r->Remote(i);
        const auto status = r->network.Status(i);
        number("retired", remote.retired);
        number("cleanup_failed", remote.cleanup_failed);
        number("accepted",
               r->players[i].Progress(r->clock.Completed()).accepted_count);
        number("net_rx", status.received_packets);
        number("net_tx", status.sent_packets);
        out += prefix + "join=" + JoinStateName(remote.join.state) + "\n";
        out += prefix + "peer=" + net::protocol::PeerStateName(status.state) +
               "\n";
        out += prefix + "terminal=" + net::TerminalReasonName(status.terminal) +
               "\n";
      }
      for (unsigned b = 0; b < r->topology[i].body_count; ++b) {
        const auto &body = r->topology[i].bodies[b];
        const auto key = prefix + "body." + std::to_string(b) + ".";
        n((key + "entity_id").c_str(), WorldEntityLocalId(body.player));
        n((key + "address").c_str(), reinterpret_cast<uintptr_t>(body.player));
        n((key + "native_index").c_str(), body.index);
        n((key + "native_activity_required").c_str(),
          body.native_activity_required);
        n((key + "native_activity_seen").c_str(), body.native_activity_seen);
        n((key + "role").c_str(), unsigned(body.role));
        n((key + "active").c_str(), body.CanConsume());
        n((key + "ghost").c_str(), body.ghost);
        n((key + "dead").c_str(), body.dead);
        n((key + "character").c_str(), body.character);
      }
    }
  n("active_bodies", body_total);
  n("generation", r ? SessionGeneration(*r) : 0);
  n("session_generation", r ? SessionGeneration(*r) : 0);
  n("completed_tick", r ? r->clock.Completed() : 0);
  // 只读描述最后实际提交owner；诊断/夹具可区分普通窗口与仅部分身体执行的Odd窗口。
  n("completed_standard", r && r->clock.Completed() && r->boundary.kind==core::TickKind::StandardRoom);
  n("completed_odd", r && r->clock.Completed() && r->boundary.kind==core::TickKind::OddPlayer);
  n("armed", Armed());
  n("current_tick", r ? r->pipeline.SimulationTick() : 0);
  n("ticks_standard", r ? r->ticks_standard : 0);
  n("ticks_odd", r ? r->ticks_odd : 0);
  // 阶段0计时：对照ticks_standard/ticks_odd判断Authority tick为约60Hz还是约30Hz。
  n("native_interpolation", NativeInterpolationState());
  n("a_bound", r && !r->finished && r->players[0].BoundValid());
  n("a_generation", r ? r->players[0].Generation() : 0);
  n("blocks", r ? r->blocks : 0);
  n("queue_depth_a", r ? r->players[0].Depth() : 0);
  n("query_overrides", r ? r->overrides : 0);
  n("shooting_suppressed", r ? r->suppressed : 0);
  n("odd_patch_installed", OddPatchInstalled());
  n("b_bound", r && !r->finished && r->mode == core::SessionMode::Host &&
                   r->players[1].BoundValid());
  n("b_generation",
    r && r->mode == core::SessionMode::Host ? r->players[1].Generation() : 0);
  n("queue_depth_b", r ? r->players[1].Depth() : 0);
  // B 的生命状态与重绑计数：Alive 只表示生命可用，是否绑定由 b_bound 表达。
  n("b_deaths", r ? r->life.deaths : 0);
  n("b_revives", r ? r->life.revives : 0);
  n("b_rebinds", r ? r->life.rebinds : 0);
  n("b_death_event", r ? r->life.death_event : 0);
  n("b_lost_loop", r ? r->life.lost_loop : 0);
  n("b_revived_loop", r ? r->life.revived_loop : 0);
  n("b_unreadable", r ? r->life.unreadable : 0);
  n("b_conflicts", r ? r->life.conflicts : 0);
  n("b_awaiting_rebind", r && !r->finished && r->life.awaiting_rebind);
  // 当前归属只在有效会话内展示；历史地址不解引用，退休后不冒充仍有 B。
  const bool current_b =
      r && !r->finished && r->mode == core::SessionMode::Host && r->life.joined;
  n("b_ownership_confirmed", current_b && r->life.current.confirmed);
  n("b_ownership_generation", current_b ? r->life.current.generation : 0);
  n("b_lost_tick", r ? r->life.lost_tick : 0);
  n("b_revived_tick", r ? r->life.revived_tick : 0);
  out +=
      "b_pending_reason=" + std::string(r ? r->life.pending_reason : "") + "\n";
  out += "b_conflict_reason=" + std::string(r ? r->life.conflict_reason : "") +
         "\n";
  // admission、接受、Resolve/正常返回进度分别计数，不拿 attempted 代替
  // Accepted。
  const auto progress = r && r->mode == core::SessionMode::Host
                            ? r->players[1].Progress(r->clock.Completed())
                            : core::ClientInputProgress{};
  n("peer_accepted_commands", progress.accepted_count);
  n("peer_duplicate_commands", progress.duplicate_count);
  n("peer_rejected_commands", progress.rejected_count);
  n("peer_accepted_sequence", progress.last_accepted_sequence);
  n("peer_processed_sequence", progress.last_processed_sequence);
  n("peer_advertised_generation", r ? r->peer.advertised_generation : 0);
  // 已退休时保留最后一轮诊断，但 Player 数必须来自当前世界，不能沿用移除前的
  // 2。
  n("players", ReadTopology().player_count);
  n("device_k_id", r && r->device.Object() ? r->device.Id() : 0);
  n("host_patches_installed", HostPatchesInstalled());
  n("peer_discarded_commands", r ? r->peer.discarded : 0);
  n("peer_admitted_commands", r ? r->peer.admitted : 0);
  n("peer_ack_dropped", r ? r->peer.ack_dropped : 0);
  n("peer_ack_unknown", r ? r->peer.ack_unknown : 0);
  // 按原因的远端丢弃：与 Accepted 分开，admitted 仍只是尝试数。
  for (std::size_t i = 0; i < static_cast<std::size_t>(DiscardReason::Count);
       ++i) {
    const auto key = std::string("peer_discard.") +
                     DiscardReasonName(static_cast<DiscardReason>(i));
    n(key.c_str(), r ? r->peer.discards[i] : 0);
  }
  n("cleanup_failed", r && r->cleanup_failed);
  const auto network = r ? r->network.Status() : net::WorkerStatus{};
  n("net_rx", network.received_packets);
  n("net_tx", network.sent_packets);
  n("net_duplicate", network.duplicate_packets);
  n("net_stale", network.stale_packets);
  n("net_rx_dropped", network.receive_queue_dropped);
  n("net_tx_dropped", network.send_queue_dropped);
  n("net_foreign", network.foreign_packets);
  n("net_route_rejected", network.routing_rejected_packets);
  n("net_rx_errors", network.receive_errors);
  n("net_tx_errors", network.send_errors);
  n("net_dropped", network.foreign_packets + network.stale_packets +
                       network.receive_queue_dropped +
                       network.send_queue_dropped);
  n("net_valid", network.valid_packets);
  n("net_rx_bytes", network.received_bytes);
  n("net_tx_bytes", network.sent_bytes);
  // 每Peer线上预算与可靠操作聚合；生产不开逐包明细，也能归约三端总上行。
  if(r)for(unsigned ordinal=1;ordinal<core::kMaximumParticipants;++ordinal){
    const auto status=r->network.Status(ordinal);const auto prefix="stream.peer."+std::to_string(ordinal)+".";
    n((prefix+"wire_bytes").c_str(),status.uplink_wire_bytes);n((prefix+"budget_blocks").c_str(),status.uplink_budget_blocks);
    for(unsigned lane=0;lane<4;++lane)n((prefix+"lane."+std::to_string(lane)).c_str(),status.uplink_lane_bytes[lane]);
    n((prefix+"phase").c_str(),status.publication_phase);n((prefix+"retransmissions").c_str(),status.operation_retransmissions);n((prefix+"cancellations").c_str(),status.operation_cancellations);n((prefix+"completed").c_str(),status.operation_completed);n((prefix+"failed").c_str(),status.operation_failed);
    n((prefix+"binary_world_ack").c_str(),status.binary_world_ack);n((prefix+"binary_body_ack").c_str(),status.binary_body_ack);n((prefix+"binary_event_ack").c_str(),status.binary_event_ack);
    n((prefix+"binary_event_dropped_capacity").c_str(),status.binary_event_dropped_capacity);n((prefix+"binary_event_dropped_expired").c_str(),status.binary_event_dropped_expired);n((prefix+"binary_unmatched_acks").c_str(),status.binary_unmatched_acks);
    n((prefix+"binary_joint_sent").c_str(),status.binary_joint_sent);
    n((prefix+"binary_world_sent").c_str(),status.binary_world_sent);n((prefix+"binary_body_sent").c_str(),status.binary_body_sent);
    n((prefix+"binary_body_dependency_blocks").c_str(),status.binary_body_dependency_blocks);
    n((prefix+"binary_world_window_skips").c_str(),status.binary_world_window_skips);
    n((prefix+"binary_world_inflight_max").c_str(),status.binary_world_inflight_max);
    n((prefix+"binary_world_retries").c_str(),status.binary_world_retries);
    out+=prefix+"first_failure="+status.replication_first_failure.data()+"\n";
  }

  n("net_bye_attempts", network.bye_send_attempts);
  n("net_bye_sent", network.bye_sent_packets);
  n("net_native_error", network.native_error);
  n("net_last_send_error", network.last_send_error);
  n("peer_locked", network.peer_locked);
  n("peer_running", network.running);
  n("peer_stop_finished", r && r->peer.stop_finished);
  for (size_t i = 0; i < network.errors.size(); ++i) {
    const auto key = std::string("net_error.") +
                     net::protocol::ErrorName(net::protocol::Error(i));
    n(key.c_str(), network.errors[i]);
  }
  n("peer_rtt_ms", network.rtt_last_ms);
  n("join_passes", r ? r->join.passes : 0);
  n("join_mode_calls", r ? r->join.mode_calls : 0);
  n("join_baby_overrides", r ? r->join.baby_overrides : 0);
  n("join_creations", r ? r->join.creation_calls : 0);
  out +=
      "peer_state=" + std::string(net::protocol::PeerStateName(network.state)) +
      "\n";
  out += "peer_terminal=" +
         std::string(net::TerminalReasonName(network.terminal)) + "\n";
  out += "peer_address=" + std::to_string(network.peer_address[0]) + "." +
         std::to_string(network.peer_address[1]) + "." +
         std::to_string(network.peer_address[2]) + "." +
         std::to_string(network.peer_address[3]) + ":" +
         std::to_string(network.peer_port) + "\n";
  out += "device_k_state=" +
         std::string(
             DeviceKStateName(r ? r->device.State() : DeviceKState::Empty)) +
         "\n";
  // 无会话时 join_state 显示 None；会话内保留实际业务状态，退休后由
  // ResetJoinStateAfterRetirement 复位为 None。
  out += "join_state=" +
         std::string(JoinStateName(r && r->mode == core::SessionMode::Host
                                       ? r->join.state
                                       : JoinState::None)) +
         "\n";
  out += "join_last=" +
         std::string(JoinStateName(r && r->mode == core::SessionMode::Host
                                       ? r->join.last
                                       : JoinState::None)) +
         "\n";
  out += "b_life=" +
         std::string(current_b ? RemoteLifeStatusName(r->life.status)
                               : RemoteLifeStatusName(RemoteLifeStatus::None)) +
         "\n";
  out += "b_character=" +
         std::to_string(
             r && r->mode == core::SessionMode::Host ? r->join.character : -1) +
         "\n";
  // 重复查询按场景累计；逐键细节在 query_repeat/query_coverage trace。
  n("query_reads", r ? r->query.reads : 0);
  n("query_repeats", r ? r->query.repeats : 0);
  n("query_mismatches", r ? r->query.mismatches : 0);
  n("query_overflow", r ? r->query.overflow : 0);
  n("query_nonfinite", r ? r->query.nonfinite : 0);
  n("query_epoch", r ? r->query_epoch : 0);
  n("lifecycle_full_reads", r ? r->lifecycle_refreshes : 0);
  n("lifecycle_snapshot_reuses", r ? r->lifecycle_reuses : 0);
  n("query_snapshots", r ? r->query_snapshots : 0);
  n("query_mismatch_known", r && r->query.mismatch_known);
  {
    // 完整分类覆盖：按 ordinal/consumer/action/query 统计实际出现的键。
    unsigned observed = 0, repeated = 0;
    if (r)
      for (const auto &participant : r->query.coverage)
        for (const auto &consumer : participant)
          for (const auto &action : consumer)
            for (const auto &value : action) {
              observed += value.reads != 0;
              repeated += value.repeats != 0;
            }
    n("query_keys_observed", observed);
    n("query_keys_repeated", repeated);
  }
  if (r && r->query.mismatch_known) {
    n("query_mismatch_ordinal", r->query.mismatch_key.ordinal);
    n("query_mismatch_consumer", r->query.mismatch_key.consumer);
    n("query_mismatch_kind", r->query.mismatch_key.query);
    n("query_mismatch_action", r->query.mismatch_key.action);
    n("query_mismatch_tick", r->query.mismatch_key.tick);
    n("query_mismatch_generation", r->query.mismatch_key.generation);
    n("query_mismatch_epoch", r->query.mismatch_key.epoch);
    out += "query_mismatch_first=" + std::to_string(r->query.mismatch_first) +
           "\n";
    out +=
        "query_mismatch_last=" + std::to_string(r->query.mismatch_last) + "\n";
  }
  out += LoadingFields(r);
  out += PersistenceFields();
  out += PersistenceWindowFields();
  out += "retirement_cause=" +
         std::string(core::SessionCauseName(r ? r->retirement_cause
                                              : core::SessionCause::None)) +
         "\n";
  out += "last_tick_decision=" +
         std::string(core::TickDecisionName(r ? r->last_decision
                                              : core::TickDecision::Idle)) +
         "\n";
  out += "session_error=" + std::string(r ? r->last_error : "") + "\n";
  return out;
}
} // namespace cs::game
