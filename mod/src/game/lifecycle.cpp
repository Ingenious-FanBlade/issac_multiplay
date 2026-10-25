#include "game/lifecycle.h"
#include "game/context.h"
#include "game/input_a.h"
#include "game/state.h"
#include "game/tick.h"
#include <algorithm>

namespace cs::game {
namespace {
// B 的死亡/未知关联只撤销 B；Active 的 Authority 不因远端未绑定而等待。
// 仍使用 Participant 原有 Invalidate 和中性 Resolve，不建立另一条输入路径。
void InvalidateRemote(SessionRuntime &r, const char *reason) noexcept {
  if (r.players[1].BoundValid()) {
    r.controlled_player[1] = r.players[1].Bound().player;
    r.players[1].Invalidate();
    Trace("binding", "ordinal=1 bound=0 generation=%llu reason=%s loop=%llu",
          r.players[1].Generation(), reason, r.loop);
  }
  DiscardPeerWindow(r);
}
} // namespace
// 两个 ordinal 对称失效；旧地址只用于识别中性查询，绝不据此解引用旧 Player。
void InvalidateBinding(SessionRuntime &r, const char *reason,
                       bool defer) noexcept {
  for (unsigned i = 0; i < r.session.ParticipantCount(); ++i) {
    if (r.players[i].BoundValid()) {
      r.controlled_player[i] = r.players[i].Bound().player;
      r.players[i].Invalidate();
      Trace("binding", "ordinal=%u bound=0 generation=%llu reason=%s loop=%llu",
            i, r.players[i].Generation(), reason, r.loop);
    }
  }
  if (defer)
    r.defer_through = (std::max)(r.defer_through, r.loop);
  DiscardInputEdges();
  DiscardPeerWindow(r);
}
void RefreshLifecycle(SessionRuntime &r, bool authority) noexcept {
  if (r.finished || r.retiring || r.join.scope ||
      (r.mode == core::SessionMode::Host && !r.peer.established))
    return;
  std::array<Topology, 2> values{ReadTopology(), {}};
  const bool remote = r.mode == core::SessionMode::Host && r.join.complete;
  if (remote)
    values[1] = ReadTopology(1, r.device.Object());
  const auto &value = values[0];
  if (value.known && !value.run) {
    // 主菜单握手后的 Warmup 无限等待首次 run；真正经历 run 后离开才是
    // teardown。
    if (r.mode == core::SessionMode::Host && !r.run_observed) {
      r.topology = values;
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
  if (value.run)
    r.run_observed = true;
  if (r.mode == core::SessionMode::Host && value.known && value.run &&
      value.player_count && value.player_count != (remote ? 2u : 1u)) {
    FailRuntime("host_player_count_changed");
    return;
  }
  const auto old_blocks = r.blocks;
  unsigned blocks = 0;
  if (!value.known || !value.unique || !value.Alive() || !ContextHealthy())
    blocks |= Lifecycle;
  if (remote) {
    if (values[1].dead || values[1].removed || values[1].ghost)
      r.remote_life_lost = true;
    if (!values[1].known || !values[1].unique || !values[1].Alive() ||
        r.remote_life_lost)
      InvalidateRemote(r, "remote_lifecycle_unavailable");
  }
  if (r.console_blocked || value.pause > 0 ||
      (!value.focused && value.pause_on_focus_lost))
    blocks |= Pause;
  if (value.starting || value.room_transition || value.stage_transition)
    blocks |= Loading;
  if (r.observed) {
    for (unsigned i = 0; i < (remote ? 2u : 1u); ++i) {
      if (!(values[i].room == r.topology[i].room))
        InvalidateBinding(r, "room_identity_changed", true);
      else if (!SameLife(values[i], r.topology[i])) {
        if (i == 1)
          InvalidateRemote(r, "player_lifecycle_changed");
        else
          InvalidateBinding(r, "player_lifecycle_changed", true);
      } else if (!(values[i].pair == r.topology[i].pair) ||
                 values[i].unique != r.topology[i].unique) {
        if (i == 1)
          InvalidateRemote(r, "exact_pair_changed");
        else
          InvalidateBinding(r, "exact_pair_changed", true);
      }
    }
  }
  if ((blocks & ~old_blocks) != 0)
    InvalidateBinding(r, "block_enter", false);
  if ((old_blocks & ~blocks) != 0)
    InvalidateBinding(r, "block_resume", false);
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
  if (authority && !blocks) {
    for (unsigned i = 0; i < (remote ? 2u : 1u); ++i) {
      if (r.players[i].BoundValid())
        continue;
      if (!values[i].unique || !values[i].Alive() ||
          (i == 1 && r.remote_life_lost))
        continue;
      if (!r.players[i].Bind(values[i].pair)) {
        FailRuntime("unique_binding_rejected");
        return;
      }
      r.controlled_player[i] = values[i].pair.player;
      if (i == 1)
        DiscardPeerWindow(r);
      Trace("binding",
            "ordinal=%u bound=1 generation=%llu loop=%llu "
            "reason=stable_authority",
            i, r.players[i].Generation(), r.loop);
    }
  }
}
// Warmup 的稳定 Authority 只负责 K 的一次创建/等待 Join，不分配输入或推进
// tick。
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
    if (!r.join.complete ||
        (r.session.Status().state == core::SessionState::Warmup &&
         !r.players[1].BoundValid()))
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
void RunTeardownBeginning() noexcept {
  if (!IsMain())
    return;
  if (auto *r = Runtime(); r && !r->finished)
    r->run_torn_down = true;
  StopSession(core::SessionCause::RunTeardown);
}
void RestoreBeginning() noexcept { RunTeardownBeginning(); }
} // namespace cs::game
