#include "game/lifecycle.h"
#include "game/context.h"
#include "game/input_a.h"
#include "game/state.h"
#include "game/tick.h"
#include <algorithm>

namespace cs::game {
// 失效对象仍保留在 controlled_player 中，只用于识别中性查询，不解引用它。
void InvalidateBinding(SoloRuntime &r, const char *reason,
                       bool defer) noexcept {
  if (r.players[0].BoundValid()) {
    r.controlled_player = r.players[0].Bound().player;
    r.players[0].Invalidate();
    Trace("binding", "ordinal=0 bound=0 generation=%llu reason=%s loop=%llu",
          r.players[0].Generation(), reason, r.loop);
  }
  if (defer)
    r.defer_through = (std::max)(r.defer_through, r.loop);
  DiscardInputEdges();
}
void RefreshLifecycle(SoloRuntime &r, bool authority) noexcept {
  if (r.finished || r.retiring)
    return;
  const auto value = ReadTopology();
  if (value.known && !value.run) {
    StopSolo(core::SessionCause::RunTeardown);
    return;
  }
  const auto old_blocks = r.blocks;
  unsigned blocks = 0;
  if (!value.known || !value.unique || !value.Alive() || !ContextHealthy())
    blocks |= Lifecycle;
  if (r.console_blocked || value.pause > 0 ||
      (!value.focused && value.pause_on_focus_lost))
    blocks |= Pause;
  if (value.starting || value.room_transition || value.stage_transition)
    blocks |= Loading;
  if (r.observed) {
    if (!(value.room == r.topology.room))
      InvalidateBinding(r, "room_identity_changed", true);
    else if (!SameLife(value, r.topology))
      InvalidateBinding(r, "player_lifecycle_changed", true);
    else if (!(value.pair == r.topology.pair) ||
             value.unique != r.topology.unique)
      InvalidateBinding(r, "exact_pair_changed", true);
  }
  if ((blocks & ~old_blocks) != 0)
    InvalidateBinding(r, "block_enter", false);
  if ((old_blocks & ~blocks) != 0)
    InvalidateBinding(r, "block_resume", false);
  if (r.loop <= r.defer_through)
    blocks |= Lifecycle;
  r.blocks = blocks;
  r.topology = value;
  r.observed = true;
  if (old_blocks != blocks)
    Trace("block_interval",
          "from=%u to=%u loop=%llu console=%u pause=%d focus=%u "
          "room_transition=%d stage_transition=%d reason=%s",
          old_blocks, blocks, r.loop, r.console_blocked, value.pause,
          value.focused, value.room_transition, value.stage_transition,
          value.reason);
  if (authority && !blocks && value.unique && !r.players[0].BoundValid()) {
    if (!r.players[0].Bind(value.pair)) {
      FailRuntime("unique_binding_rejected");
      return;
    }
    r.controlled_player = value.pair.player;
    Trace("binding",
          "ordinal=0 bound=1 generation=%llu loop=%llu reason=stable_authority",
          r.players[0].Generation(), r.loop);
  }
}
bool ReadyForAuthority(SoloRuntime &r) noexcept {
  RefreshLifecycle(r, true);
  if (r.retiring || r.blocks || !r.players[0].BoundValid())
    return false;
  if (r.session.Status().state == core::SessionState::Warmup) {
    if (!r.session.Activate(true)) {
      FailRuntime("activate_failed");
      return false;
    }
    Trace(
        "session",
        "from=Warmup to=Active reason=BindingsReady generation=%llu loop=%llu",
        r.session.Identity().generation, r.loop);
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
void RestoreBeginning() noexcept {
  if (IsMain())
    StopSolo(core::SessionCause::RunTeardown);
}
} // namespace cs::game
